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
#include "utils/coord_parser.h"
#include "asgard/context.h"
#include "asgard/direct_path_response_builder.h"
#include "asgard/metrics.h"
#include "asgard/projector.h"
#include "asgard/request.pb.h"
#include "asgard/util.h"

#include <valhalla/midgard/pointll.h>
#include <valhalla/odin/directionsbuilder.h>
#include <valhalla/thor/attributes_controller.h>
#include <valhalla/thor/triplegbuilder.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/range/algorithm/for_each.hpp>
#include <boost/range/join.hpp>

#include <ctime>
#include <numeric>
#include <utility>

using namespace valhalla;

namespace asgard {

using ValhallaLocations = google::protobuf::RepeatedPtrField<valhalla::Location>;
using ProjectedLocations = std::unordered_map<midgard::PointLL, valhalla::baldr::PathLocation>;

// The max size of matrix in jormungandr is 5000 so far...
constexpr size_t MAX_MASK_SIZE = 10000;
using ProjectionFailedMask = std::bitset<MAX_MASK_SIZE>;

namespace {

pbnavitia::Response make_error_response(pbnavitia::Error_error_id err_id, const std::string& err_msg) {
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
    }
    if (mode == "bike") {
        return duration * kTimeDistCostThresholdBicycleDivisor;
    }
    return duration * kTimeDistCostThresholdAutoDivisor;
}

double get_speed_request(const pbnavitia::Request& request, const std::string& mode) {
    auto const& request_params = request.direct_path().streetnetwork_params();

    if (mode == "walking") {
        return request_params.walking_speed();
    }
    if (mode == "bike") {
        return request_params.bike_speed();
    }
    if (mode == "car") {
        return request_params.car_speed();
    }
    if (mode == "taxi") {
        return request_params.car_no_park_speed();
    }
    throw std::invalid_argument("Bad get_speed_request parameter");
}

std::pair<ValhallaLocations, ProjectionFailedMask>
make_valhalla_locations_from_projected_locations(const std::vector<midgard::PointLL>& navitia_locations,
                                                 const ProjectedLocations& projected_locations,
                                                 valhalla::baldr::GraphReader& graph) {
    ValhallaLocations valhalla_locations;
    // This mask is used to remember the index of navitia locations whose projection has failed
    // 0 means projection OK, 1 means projection KO
    ProjectionFailedMask projection_failed_mask;

    size_t source_idx = -1;
    for (const auto& l : navitia_locations) {
        ++source_idx;
        auto it = projected_locations.find(l);
        if (it == projected_locations.end()) {
            projection_failed_mask.set(source_idx);
            LOG_ERROR("Cannot project coord: " + std::to_string(l.lng()) + ";" + std::to_string(l.lat()));
            continue;
        }
        baldr::PathLocation::toPBF(it->second, valhalla_locations.Add(), graph);
    }

    return std::make_pair(std::move(valhalla_locations), projection_failed_mask);
}

} // namespace

Handler::Handler(const Context& context) : graph(context.graph),
                                           metrics(context.metrics),
                                           projector(context.projector) {
}

pbnavitia::Response Handler::handle(const pbnavitia::Request& request) {
    switch (request.requested_api()) {
    case pbnavitia::street_network_routing_matrix: return handle_matrix(request);
    case pbnavitia::direct_path: return handle_direct_path(request);
    default:
        LOG_ERROR("wrong request: aborting");
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

    const auto navitia_sources = util::convert_locations_to_pointLL(request.sn_routing_matrix().origins());
    const auto navitia_targets = util::convert_locations_to_pointLL(request.sn_routing_matrix().destinations());

    mode_costing.update_costing_for_mode(mode, request.sn_routing_matrix().speed());
    const auto costing = mode_costing.get_costing_for_mode(mode);

    // We use the cache only when there are more than one element in the sources/targets, so the cache will keep only stop_points coord
    bool use_cache = (navitia_sources.size() > 1);
    const auto projected_sources_locations = projector(begin(navitia_sources), end(navitia_sources), graph, mode, costing, use_cache);
    if (projected_sources_locations.empty()) {
        LOG_ERROR("All sources projections failed!");
        return make_error_response(pbnavitia::Error::no_origin, "origins projection failed!");
    }

    use_cache = (navitia_targets.size() > 1);
    const auto projected_targets_locations = projector(begin(navitia_targets), end(navitia_targets), graph, mode, costing, use_cache);
    if (projected_targets_locations.empty()) {
        LOG_ERROR("All targets projections failed!");
        return make_error_response(pbnavitia::Error::no_destination, "destinations projection failed!");
    }

    LOG_INFO("Projecting locations done.");

    ValhallaLocations valhalla_location_sources;
    ProjectionFailedMask projection_mask_sources;
    std::tie(valhalla_location_sources, projection_mask_sources) = make_valhalla_locations_from_projected_locations(navitia_sources, projected_sources_locations, graph);

    ValhallaLocations valhalla_location_targets;
    ProjectionFailedMask projection_mask_targets;
    std::tie(valhalla_location_targets, projection_mask_targets) = make_valhalla_locations_from_projected_locations(navitia_targets, projected_targets_locations, graph);

    LOG_INFO(std::to_string(projection_mask_sources.count()) + " origin(s) projection failed " +
             std::to_string(projection_mask_targets.count()) + " target(s) projection failed");

    LOG_INFO("Projection Done");
    LOG_INFO("Computing matrix...");
    const auto res = matrix.SourceToTarget(valhalla_location_sources,
                                           valhalla_location_targets,
                                           graph,
                                           mode_costing.get_costing(),
                                           util::convert_navitia_to_valhalla_mode(mode),
                                           get_distance(mode, request.sn_routing_matrix().max_duration()));
    LOG_INFO("Computing matrix done.");

    pbnavitia::Response response;
    int nb_unreached = 0;
    //in fact jormun don't want a real matrix, only a vector of solution :(
    auto* row = response.mutable_sn_routing_matrix()->add_rows();
    assert(res.size() == valhalla_location_sources.size() * valhalla_location_targets.size());

    const auto& failed_projection_mask = (navitia_sources.size() == 1) ? projection_mask_targets : projection_mask_sources;
    size_t elt_idx = -1;
    size_t resp_row_size = navitia_sources.size() == 1 ? navitia_targets.size() : navitia_sources.size();
    assert(resp_row_size == failed_projection_mask.count() + res.size());

    auto res_it = res.cbegin();
    while (++elt_idx < resp_row_size) {
        auto* k = row->add_routing_response();
        if (failed_projection_mask[elt_idx]) {
            k->set_duration(-1);
            k->set_routing_status(pbnavitia::RoutingStatus::unreached);
            ++nb_unreached;
        } else if (res_it != res.cend()) {
            k->set_duration(res_it->time);
            if (res_it->time == thor::kMaxCost ||
                res_it->time > uint32_t(request.sn_routing_matrix().max_duration())) {
                k->set_routing_status(pbnavitia::RoutingStatus::unreached);
                ++nb_unreached;
            } else {
                k->set_routing_status(pbnavitia::RoutingStatus::reached);
            }
            ++res_it;
        }
    }

    LOG_INFO("Request done with " + std::to_string(nb_unreached) + " unreached");

    if (graph.OverCommitted()) { graph.Clear(); }
    matrix.Clear();
    LOG_INFO("Everything is clear.");

    const auto duration = pt::microsec_clock::universal_time() - start;
    metrics.observe_handle_matrix(mode, duration.total_milliseconds() / 1000.0);
    metrics.observe_nb_cache_miss(projector.get_nb_cache_miss(), projector.get_nb_cache_calls());

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

    std::vector<midgard::PointLL> locations = util::convert_locations_to_pointLL(std::vector<pbnavitia::LocationContext>{request.direct_path().origin(),
                                                                                                                         request.direct_path().destination()});

    LOG_INFO("Projecting locations...");
    // It's a direct path.. we don't pollute the cache with random coords...
    const bool use_cache = false;
    auto projected_locations = projector(begin(locations), end(locations), graph, mode, costing, use_cache);
    LOG_INFO("Projecting locations done.");

    if (projected_locations.size() != 2) {
        return make_error_response(pbnavitia::Error::no_origin_nor_destination, "Cannot project the given coords!");
    }

    valhalla::Location origin;
    valhalla::Location dest;
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
    valhalla::Api api;
    auto* trip_leg = api.mutable_trip()->mutable_routes()->Add()->mutable_legs()->Add();
    thor::TripLegBuilder::Build(controller, graph, mode_costing.get_costing(), pathedges.begin(),
                                pathedges.end(), origin, dest, {}, *trip_leg);

    api.mutable_options()->set_language(request.direct_path().streetnetwork_params().language());
    odin::DirectionsBuilder::Build(api);

    const auto response = direct_path_response_builder::build_journey_response(request, pathedges, *trip_leg, api);

    if (graph.OverCommitted()) { graph.Clear(); }
    algo.Clear();
    LOG_INFO("Everything is clear.");

    auto duration = pt::microsec_clock::universal_time() - start;
    metrics.observe_handle_direct_path(mode, duration.total_milliseconds() / 1000.0);
    return response;
}

} // namespace asgard
