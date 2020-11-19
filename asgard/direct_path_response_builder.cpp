#include "asgard/direct_path_response_builder.h"
#include "asgard/request.pb.h"
#include "asgard/util.h"

#include <valhalla/midgard/encoded.h>
#include <valhalla/midgard/logging.h>
#include <valhalla/midgard/pointll.h>
#include <valhalla/thor/pathinfo.h>

#include <numeric>

using namespace valhalla;

namespace asgard {

namespace direct_path_response_builder {

constexpr static float KM_TO_M = 1000.f;

pbnavitia::Response build_journey_response(const pbnavitia::Request& request,
                                           const std::vector<valhalla::thor::PathInfo>& pathedges,
                                           const TripLeg& trip_leg,
                                           valhalla::Api& api) {
    pbnavitia::Response response;

    if (pathedges.empty()) {
        response.set_response_type(pbnavitia::NO_SOLUTION);
        LOG_ERROR("No solution found !");
        return response;
    }

    LOG_INFO("Building solution...");
    // General
    response.set_response_type(pbnavitia::ITINERARY_FOUND);

    // Journey
    auto* journey = response.mutable_journeys()->Add();
    journey->set_duration(pathedges.back().elapsed_time);
    journey->set_nb_transfers(0);
    journey->set_nb_sections(1);

    auto const departure_posix_time = time_t(request.direct_path().datetime());
    auto const arrival_posix_time = departure_posix_time + time_t(journey->duration());

    journey->set_requested_date_time(departure_posix_time);
    journey->set_departure_date_time(departure_posix_time);
    journey->set_arrival_date_time(arrival_posix_time);

    // Section
    auto* s = journey->add_sections();
    s->set_type(pbnavitia::STREET_NETWORK);
    s->set_id("section_0");
    s->set_duration(journey->duration());
    // We take the mode of the first path. Could be the last too...
    // They could also be different in the list...
    s->mutable_street_network()->set_mode(util::convert_valhalla_to_navitia_mode(pathedges.front().mode));
    s->set_begin_date_time(departure_posix_time);
    s->set_end_date_time(arrival_posix_time);

    auto total_length = std::accumulate(
        trip_leg.node().begin(),
        trip_leg.node().end(),
        0.f,
        [&](float sum, const TripLeg_Node& node) { return sum + node.edge().length() * KM_TO_M; });
    s->set_length(total_length);

    auto const list_geo_points = midgard::decode<std::vector<midgard::PointLL>>(trip_leg.shape());

    set_extremity_pt_object(list_geo_points.front(), s->mutable_origin());
    set_extremity_pt_object(list_geo_points.back(), s->mutable_destination());
    compute_geojson(list_geo_points, *s);
    bool enable_instructions = request.direct_path().streetnetwork_params().enable_instructions();
    compute_path_items(api, s->mutable_street_network(), enable_instructions);

    compute_metadata(*journey);

    LOG_INFO("Solution built...");

    return response;
}

void set_extremity_pt_object(const valhalla::midgard::PointLL& geo_point, pbnavitia::PtObject* o) {
    auto uri = std::stringstream();
    uri << std::fixed << std::setprecision(5) << geo_point.lng() << ";" << geo_point.lat();
    o->set_uri(uri.str());
    o->set_name("");
    auto* coords = o->mutable_address()->mutable_coord();
    coords->set_lat(geo_point.lat());
    coords->set_lon(geo_point.lng());
}

void compute_geojson(const std::vector<midgard::PointLL>& list_geo_points, pbnavitia::Section& s) {
    for (const auto& point : list_geo_points) {
        auto* geo = s.mutable_street_network()->add_coordinates();
        geo->set_lat(point.lat());
        geo->set_lon(point.lng());
    }
}

void compute_metadata(pbnavitia::Journey& pb_journey) {
    uint32_t total_walking_duration = 0;
    uint32_t total_car_duration = 0;
    uint32_t total_bike_duration = 0;
    uint32_t total_ridesharing_duration = 0;
    uint32_t total_taxi_duration = 0;

    uint32_t total_walking_distance = 0;
    uint32_t total_car_distance = 0;
    uint32_t total_bike_distance = 0;
    uint32_t total_ridesharing_distance = 0;
    uint32_t total_taxi_distance = 0;

    for (const auto& section : pb_journey.sections()) {
        if (section.type() == pbnavitia::STREET_NETWORK || section.type() == pbnavitia::CROW_FLY) {
            switch (section.street_network().mode()) {
            case pbnavitia::StreetNetworkMode::Walking:
                total_walking_duration += section.duration();
                total_walking_distance += section.length();
                break;
            case pbnavitia::StreetNetworkMode::Car:
            case pbnavitia::StreetNetworkMode::CarNoPark:
                total_car_duration += section.duration();
                total_car_distance += section.length();
                break;
            case pbnavitia::StreetNetworkMode::Bike:
            case pbnavitia::StreetNetworkMode::Bss:
                total_bike_duration += section.duration();
                total_bike_distance += section.length();
                break;
            case pbnavitia::StreetNetworkMode::Ridesharing:
                total_ridesharing_duration += section.duration();
                total_ridesharing_distance += section.length();
                break;
            case pbnavitia::StreetNetworkMode::Taxi:
                total_taxi_duration += section.duration();
                total_taxi_distance += section.length();
                break;
            }
        } else if (section.type() == pbnavitia::TRANSFER && section.transfer_type() == pbnavitia::walking) {
            total_walking_duration += section.duration();
        }
    }

    const auto ts_departure = pb_journey.sections(0).begin_date_time();
    const auto ts_arrival = pb_journey.sections(pb_journey.sections_size() - 1).end_date_time();
    auto* durations = pb_journey.mutable_durations();
    durations->set_walking(total_walking_duration);
    durations->set_bike(total_bike_duration);
    durations->set_car(total_car_duration);
    durations->set_ridesharing(total_ridesharing_duration);
    durations->set_taxi(total_taxi_duration);
    durations->set_total(ts_arrival - ts_departure);

    auto* distances = pb_journey.mutable_distances();
    distances->set_walking(total_walking_distance);
    distances->set_bike(total_bike_distance);
    distances->set_car(total_car_distance);
    distances->set_ridesharing(total_ridesharing_distance);
    distances->set_taxi(total_taxi_distance);

    pb_journey.set_duration(ts_arrival - ts_departure);
}

void compute_path_items(valhalla::Api& api, pbnavitia::StreetNetwork* sn, const bool enable_instruction) {
    if (api.mutable_trip()->routes_size() > 0 && api.has_directions() && api.mutable_directions()->mutable_routes(0)->legs_size() > 0) {
        auto& trip_route = *api.mutable_trip()->mutable_routes(0);
        auto& directions_leg = *api.mutable_directions()->mutable_routes(0)->mutable_legs(0);
        for (int i = 0; i < directions_leg.maneuver_size(); ++i) {
            const auto& maneuver = directions_leg.maneuver(i);
            auto* path_item = sn->add_path_items();
            auto const& edge = trip_route.mutable_legs(0)->node(i).edge();
            set_path_item_type(edge, *path_item);
            set_path_item_name(maneuver, *path_item);
            set_path_item_length(maneuver, *path_item);
            set_path_item_duration(maneuver, *path_item);
            set_path_item_direction(maneuver, *path_item);
            if (enable_instruction) {
                set_path_item_instruction(maneuver, *path_item, i, directions_leg.maneuver_size());
            }
        }
    }
}

void set_path_item_instruction(const DirectionsLeg_Maneuver& maneuver, pbnavitia::PathItem& path_item, const size_t index, const size_t size) {
    if (maneuver.has_text_instruction() && !maneuver.text_instruction().empty()) {
        auto instruction = maneuver.text_instruction();
        if (index != (size - 1)) {
            instruction += " Keep going for " + std::to_string((int)(maneuver.length() * KM_TO_M)) + " m.";
        }
        path_item.set_instruction(instruction);
    }
}

void set_path_item_name(const DirectionsLeg_Maneuver& maneuver, pbnavitia::PathItem& path_item) {
    if (!maneuver.street_name().empty() && maneuver.street_name().Get(0).has_value()) {
        path_item.set_name(maneuver.street_name().Get(0).value());
    }
}

void set_path_item_length(const DirectionsLeg_Maneuver& maneuver, pbnavitia::PathItem& path_item) {
    if (maneuver.has_length()) {
        path_item.set_length(maneuver.length() * KM_TO_M);
    }
}

// For now, we only handle cycle lanes
void set_path_item_type(const TripLeg_Edge& edge, pbnavitia::PathItem& path_item) {
    if (edge.has_cycle_lane()) {
        path_item.set_cycle_path_type(util::convert_valhalla_to_navitia_cycle_lane(edge.cycle_lane()));
    }
}

void set_path_item_duration(const DirectionsLeg_Maneuver& maneuver, pbnavitia::PathItem& path_item) {
    if (maneuver.has_time()) {
        path_item.set_duration(maneuver.time());
    }
}

void set_path_item_direction(const DirectionsLeg_Maneuver& maneuver, pbnavitia::PathItem& path_item) {
    if (maneuver.has_turn_degree()) {
        int turn_degree = maneuver.turn_degree() % 360;
        if (turn_degree > 180) turn_degree -= 360;
        path_item.set_direction(turn_degree);
    }
}

} // namespace direct_path_response_builder

} // namespace asgard
