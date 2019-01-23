#include "asgard/direct_path_response_builder.h"
#include "asgard/request.pb.h"
#include "asgard/util.h"

#include <valhalla/midgard/encoded.h>
#include <valhalla/midgard/logging.h>
#include <valhalla/thor/pathinfo.h>
#include <valhalla/thor/trippathbuilder.h>

#include <numeric>

using namespace valhalla;

namespace asgard {

namespace direct_path_response_builder {

pbnavitia::Response build_journey_response(const pbnavitia::Request& request,
                                           const std::vector<valhalla::thor::PathInfo>& path_info_list,
                                           const odin::TripPath& trip_path) {
    pbnavitia::Response response;

    if (path_info_list.empty()) {
        response.set_response_type(pbnavitia::NO_SOLUTION);
        LOG_ERROR("No solution found !");
        return response;
    }

    LOG_INFO("Building solution...");
    // General
    response.set_response_type(pbnavitia::ITINERARY_FOUND);

    // Journey
    auto* journey = response.mutable_journeys()->Add();
    journey->set_duration(path_info_list.back().elapsed_time);
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
    s->mutable_street_network()->set_mode(util::convert_valhalla_to_navitia_mode(path_info_list.front().mode));
    s->set_begin_date_time(departure_posix_time);
    s->set_end_date_time(arrival_posix_time);

    auto total_length = std::accumulate(
        trip_path.node().begin(),
        trip_path.node().end(),
        0.f,
        [&](float sum, const odin::TripPath_Node& node) { return sum + node.edge().length() * 1000.f; });
    s->set_length(total_length);

    auto list_geo_points = midgard::decode<std::vector<PointLL>>(trip_path.shape());
    compute_geojson(list_geo_points, *s);

    compute_metadata(*journey);

    LOG_INFO("Solution built...");

    return response;
}

void compute_geojson(const std::vector<midgard::PointLL>& list_geo_points, pbnavitia::Section& s) {
    for (const auto point : list_geo_points) {
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
    uint32_t total_walking_distance = 0;
    uint32_t total_car_distance = 0;
    uint32_t total_bike_distance = 0;
    uint32_t total_ridesharing_distance = 0;

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
    durations->set_total(ts_arrival - ts_departure);

    auto* distances = pb_journey.mutable_distances();
    distances->set_walking(total_walking_distance);
    distances->set_bike(total_bike_distance);
    distances->set_car(total_car_distance);
    distances->set_ridesharing(total_ridesharing_distance);
}
} // namespace direct_path_response_builder

} // namespace asgard
