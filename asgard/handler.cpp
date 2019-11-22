// Copyright 2017-2018, CanalTP and/or its affiliates. All rights reserved.
//
// LICENCE: This program is free software; you can redistribute it
// and/or modify it under the terms of the GNU Affero General Public
// License as published by the Free Software Foundation, either
// version 3 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public
// License along with this program. If not, see
// <http://www.gnu.org/licenses/>.

#include "asgard/handler.h"
#include "asgard/context.h"
#include "asgard/direct_path_response_builder.h"
#include "asgard/metrics.h"
#include "asgard/projector.h"
#include "asgard/request.pb.h"
#include "asgard/util.h"

#include <valhalla/thor/attributes_controller.h>
#include <valhalla/thor/triplegbuilder.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/range/join.hpp>

#include <ctime>
#include <numeric>

using namespace valhalla;

namespace asgard {

constexpr size_t MAX_MASK_SIZE = 10000;

namespace {

pbnavitia::Response make_error_response(pbnavitia::Error_error_id err_id, const std::string err_msg) {
    pbnavitia::Response error_response;
    error_response.set_response_type(pbnavitia::NO_SOLUTION);
    error_response.mutable_error()->set_id(err_id);
    error_response.mutable_error()->set_message(err_msg);
    LOG_ERROR(err_msg + " No solution found !");
    return error_response;
}

float get_distance(const std::string& mode, float duration) {
    using namespace thor;
    if (mode == "walking") {
        return duration * kTimeDistCostThresholdPedestrianDivisor;
    } else if (mode == "bike") {
        return duration * kTimeDistCostThresholdBicycleDivisor;
    } else {
        return duration * kTimeDistCostThresholdAutoDivisor;
    }
}

double get_speed_request(const pbnavitia::Request& request, const std::string& mode) {
    auto const& request_params = request.direct_path().streetnetwork_params();

    if (mode == "walking") {
        return request_params.walking_speed();
    } else if (mode == "bike") {
        return request_params.bike_speed();
    } else if (mode == "car") {
        return request_params.car_speed();
    } else if (mode == "taxi") {
        return request_params.car_no_park_speed();
    } else {
        throw std::invalid_argument("Bad get_speed_request parameter");
    }
}

using LocationContextList = google::protobuf::RepeatedPtrField<pbnavitia::LocationContext>;
std::vector<std::string> get_locations_from_matrix_request(const LocationContextList& request_locations) {
    std::vector<std::string> locations;
    locations.reserve(request_locations.size());

    for (const auto& l : request_locations) {
        locations.push_back(l.place());
    }

    return locations;
}

using ValhallaLocations = google::protobuf::RepeatedPtrField<valhalla::Location>;
using ProjectedLocations = std::unordered_map<std::string, valhalla::baldr::PathLocation>;
ValhallaLocations make_valhalla_locations_from_projected_locations(const std::vector<std::string>& navitia_locations,
                                                                   const ProjectedLocations& projected_locations,
                                                                   valhalla::baldr::GraphReader& graph) {
    ValhallaLocations valhalla_locations;
    std::bitset<MAX_MASK_SIZE> projection_mask;
    size_t source_idx = -1;
    for (const auto& l : navitia_locations) {
        ++source_idx;
        auto it = projected_locations.find(l);
        if (it == projected_locations.end()) {
            projection_mask.set(source_idx);
            continue;
        }
        baldr::PathLocation::toPBF(it->second, valhalla_locations.Add(), graph);
    }

    return valhalla_locations;
}

} // namespace

Handler::Handler(const Context& context) : graph(context.graph),
                                           matrix(),
                                           mode_costing(),
                                           metrics(context.metrics),
                                           projector(context.projector) {
}

pbnavitia::Response Handler::handle(const pbnavitia::Request& request) {
    switch (request.requested_api()) {
    case pbnavitia::street_network_routing_matrix: return handle_matrix(request);
    case pbnavitia::direct_path: return handle_direct_path(request);
    default:
        LOG_WARN("wrong request: aborting");
        return pbnavitia::Response();
    }
}

namespace pt = boost::posix_time;
pbnavitia::Response Handler::handle_matrix(const pbnavitia::Request& request) {
    pt::ptime start = pt::microsec_clock::universal_time();
    const std::string mode = request.sn_routing_matrix().mode();
    LOG_INFO("Processing matrix request " +
             std::to_string(request.sn_routing_matrix().origins_size()) + "x" +
             std::to_string(request.sn_routing_matrix().destinations_size()) +
             " with mode " + mode);

    const auto navitia_sources = get_locations_from_matrix_request(request.sn_routing_matrix().origins());
    const auto navitia_targets = get_locations_from_matrix_request(request.sn_routing_matrix().destinations());

    mode_costing.update_costing_for_mode(mode, request.sn_routing_matrix().speed());
    const auto costing = mode_costing.get_costing_for_mode(mode);

    LOG_INFO("Projecting " + std::to_string(navitia_sources.size() + navitia_targets.size()) + " locations...");
    const auto range = boost::range::join(navitia_sources, navitia_targets);
    const auto projected_locations = projector(begin(range), end(range), graph, mode, costing);
    LOG_INFO("Projecting locations done.");

    if (projected_locations.empty()) {
        return make_error_response(pbnavitia::Error::no_origin_nor_destination, "Cannot project the given coords!");
        ;
    }

    google::protobuf::RepeatedPtrField<valhalla::Location> path_location_sources;
    google::protobuf::RepeatedPtrField<valhalla::Location> path_location_targets;
    for (const auto& e : sources) {
        baldr::PathLocation::toPBF(path_locations.at(e), path_location_sources.Add(), graph);
    }
    for (const auto& e : targets) {
        baldr::PathLocation::toPBF(path_locations.at(e), path_location_targets.Add(), graph);
    }

    LOG_INFO("Computing matrix...");
    auto res = matrix.SourceToTarget(path_location_sources,
                                     path_location_targets,
                                     graph,
                                     mode_costing.get_costing(),
                                     util::convert_navitia_to_valhalla_mode(mode),
                                     get_distance(mode, request.sn_routing_matrix().max_duration()));
    LOG_INFO("Computing matrix done.");

    pbnavitia::Response response;
    int nb_unreached = 0;

    //in fact jormun don't want a real matrix, only a vector of solution :(
    auto* row = response.mutable_sn_routing_matrix()->add_rows();

    assert(res.size() == path_location_sources.size() * path_location_targets.size());

    const auto& failed_projection_mask = (sources.size() == 1) ? targets_projection_mask : sources_projection_mask;

    size_t elt_idx = -1;
    size_t row_size = sources.size() == 1 ? targets.size() : sources.size();
    auto elt_it = res.cbegin();

    while (++elt_idx < row_size) {
        auto* k = row->add_routing_response();
        if (failed_projection_mask[elt_idx]) {
            k->set_duration(-1);
            k->set_routing_status(pbnavitia::RoutingStatus::unreached);
            ++nb_unreached;
        } else if (elt_it != res.cend()) {
            k->set_duration(elt_it->time);
            if (elt_it->time == thor::kMaxCost ||
                elt_it->time > uint32_t(request.sn_routing_matrix().max_duration())) {
                k->set_routing_status(pbnavitia::RoutingStatus::unreached);
                ++nb_unreached;
            } else {
                k->set_routing_status(pbnavitia::RoutingStatus::reached);
            }
            ++elt_it;
        }
    }

    LOG_INFO("Request done with " + std::to_string(nb_unreached) + " unreached");

    if (graph.OverCommitted()) { graph.Clear(); }
    matrix.Clear();
    LOG_INFO("Everything is clear.");

    const auto duration = pt::microsec_clock::universal_time() - start;
    metrics.observe_handle_matrix(mode, duration.total_milliseconds() / 1000.0);
    return response;
}

thor::PathAlgorithm& Handler::get_path_algorithm(const std::string& mode) {
    // We use astar for walking to avoid differences between
    // The time from the matrix and the one from the direct_path
    if (mode == "walking") {
        return astar;
    }

    return bda;
}

pbnavitia::Response Handler::handle_direct_path(const pbnavitia::Request& request) {
    pt::ptime start = pt::microsec_clock::universal_time();
    const auto mode = request.direct_path().streetnetwork_params().origin_mode();
    LOG_INFO("Processing direct_path request with mode " + mode);

    const auto speed_request = get_speed_request(request, mode);
    mode_costing.update_costing_for_mode(mode, speed_request);
    auto costing = mode_costing.get_costing_for_mode(mode);

    std::vector<std::string> locations{request.direct_path().origin().place(),
                                       request.direct_path().destination().place()};

    LOG_INFO("Projecting locations...");
    auto projected_locations = projector(begin(locations), end(locations), graph, mode, costing);
    LOG_INFO("Projecting locations done.");

    if (projected_locations.empty()) {
        return make_error_response(pbnavitia::Error::no_origin_nor_destination, "Cannot project the given coords!");
    }

    Location origin;
    Location dest;
    baldr::PathLocation::toPBF(projected_locations.at(locations.front()), &origin, graph);
    baldr::PathLocation::toPBF(projected_locations.at(locations.back()), &dest, graph);

    auto& algo = get_path_algorithm(mode);

    LOG_INFO("Computing best path...");
    const auto path_info_list = algo.GetBestPath(origin,
                                                 dest,
                                                 graph,
                                                 mode_costing.get_costing(),
                                                 util::convert_navitia_to_valhalla_mode(mode));
    LOG_INFO("Computing best path done.");

    // If no solution was found
    if (path_info_list.empty()) {
        pbnavitia::Response response;
        response.set_response_type(pbnavitia::NO_SOLUTION);
        LOG_ERROR("No solution found !");
        return response;
    }

    // The path algorithms all are allowed to return more than one path now.
    // None of them do, but the api allows for it
    // We just take the first and only one then
    const auto& pathedges = path_info_list.front();
    thor::AttributesController controller;
    valhalla::TripLeg trip_leg;
    thor::TripLegBuilder::Build(controller, graph, mode_costing.get_costing(), pathedges.begin(),
                                pathedges.end(), origin, dest, {}, trip_leg);

    const auto response = direct_path_response_builder::build_journey_response(request, pathedges, trip_leg);

    if (graph.OverCommitted()) { graph.Clear(); }
    algo.Clear();
    LOG_INFO("Everything is clear.");

    auto duration = pt::microsec_clock::universal_time() - start;
    metrics.observe_handle_direct_path(mode, duration.total_milliseconds() / 1000.0);
    return response;
}

} // namespace asgard
