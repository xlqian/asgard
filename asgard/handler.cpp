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
#include "asgard/request.pb.h"
#include "asgard/util.h"

#include <valhalla/thor/attributes_controller.h>
#include <valhalla/thor/trippathbuilder.h>

#include <boost/range/join.hpp>

#include <ctime>
#include <numeric>

using namespace valhalla;

namespace asgard {

static float get_distance(const std::string& mode, float duration) {
    using namespace thor;
    if (mode == "walking") {
        return duration * kTimeDistCostThresholdPedestrianDivisor;
    } else if (mode == "bike") {
        return duration * kTimeDistCostThresholdBicycleDivisor;
    } else {
        return duration * kTimeDistCostThresholdAutoDivisor;
    }
}

static double get_speed_request(const pbnavitia::Request& request, const std::string& mode) {
    auto const& request_params = request.direct_path().streetnetwork_params();

    if (mode == "walking") {
        return request_params.walking_speed();
    } else if (mode == "bike") {
        return request_params.bike_speed();
    } else if (mode == "car") {
        return request_params.car_speed();
    } else {
        throw std::invalid_argument("Bad get_speed_request parameter");
    }
}

Handler::Handler(const Context& context) : graph(context.ptree.get_child("mjolnir")),
                                           matrix(),
                                           mode_costing(),
                                           projector(context.max_cache_size) {
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

pbnavitia::Response Handler::handle_matrix(const pbnavitia::Request& request) {
    LOG_INFO("Processing matrix request " +
             std::to_string(request.sn_routing_matrix().origins_size()) + "x" +
             std::to_string(request.sn_routing_matrix().destinations_size()));
    const std::string mode = request.sn_routing_matrix().mode();
    std::vector<std::string> sources;
    sources.reserve(request.sn_routing_matrix().origins_size());

    std::vector<std::string> targets;
    targets.reserve(request.sn_routing_matrix().destinations_size());

    for (const auto& e : request.sn_routing_matrix().origins()) {
        sources.push_back(e.place());
    }
    for (const auto& e : request.sn_routing_matrix().destinations()) {
        targets.push_back(e.place());
    }

    mode_costing.update_costing_for_mode(mode, request.sn_routing_matrix().speed());
    auto costing = mode_costing.get_costing_for_mode(mode);

    LOG_INFO("Projecting " + std::to_string(sources.size() + targets.size()) + " locations...");
    auto range = boost::range::join(sources, targets);
    auto path_locations = projector(begin(range), end(range), graph, mode, costing);
    LOG_INFO("Projecting locations done.");

    google::protobuf::RepeatedPtrField<odin::Location> path_location_sources;
    google::protobuf::RepeatedPtrField<odin::Location> path_location_targets;
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
    int nb_unknown = 0;
    int nb_unreached = 0;
    //in fact jormun don't want a real matrix, only a vector of solution :(
    auto* row = response.mutable_sn_routing_matrix()->add_rows();
    assert(res.size() == sources.size() * targets.size());
    for (const auto& elt : res) {
        auto* k = row->add_routing_response();
        k->set_duration(elt.time);
        if (elt.time == thor::kMaxCost) {
            k->set_routing_status(pbnavitia::RoutingStatus::unknown);
            ++nb_unknown;
        } else if (elt.time > uint32_t(request.sn_routing_matrix().max_duration())) {
            k->set_routing_status(pbnavitia::RoutingStatus::unreached);
            ++nb_unreached;
        } else {
            k->set_routing_status(pbnavitia::RoutingStatus::reached);
        }
    }

    LOG_INFO("Request done with " +
             std::to_string(nb_unknown) + " unknown and " +
             std::to_string(nb_unreached) + " unreached");

    if (graph.OverCommitted()) { graph.Clear(); }
    LOG_INFO("Everything is clear.");

    return response;
}

pbnavitia::Response Handler::handle_direct_path(const pbnavitia::Request& request) {
    LOG_INFO("Processing direct_path request");

    const auto mode = request.direct_path().streetnetwork_params().origin_mode();
    const auto speed_request = get_speed_request(request, mode);
    mode_costing.update_costing_for_mode(mode, speed_request);
    auto costing = mode_costing.get_costing_for_mode(mode);

    std::vector<std::string> locations{request.direct_path().origin().place(),
                                       request.direct_path().destination().place()};

    LOG_INFO("Projecting locations...");
    auto path_locations = projector(begin(locations), end(locations), graph, mode, costing);
    LOG_INFO("Projecting locations done.");

    odin::Location origin;
    odin::Location dest;
    baldr::PathLocation::toPBF(path_locations.at(locations.front()), &origin, graph);
    baldr::PathLocation::toPBF(path_locations.at(locations.back()), &dest, graph);

    LOG_INFO("Computing best path...");
    auto path_info_list = bda.GetBestPath(origin,
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

    // To compute the length
    // Can disable all options except the length here
    thor::AttributesController controller;
    auto trip_path = thor::TripPathBuilder::Build(controller, graph, mode_costing.get_costing(), path_info_list, origin,
                                                  dest, {origin, dest});

    auto response = direct_path_response_builder::build_journey_response(request, path_info_list, trip_path);

    if (graph.OverCommitted()) { graph.Clear(); }
    LOG_INFO("Everything is clear.");

    return response;
}

} // namespace asgard
