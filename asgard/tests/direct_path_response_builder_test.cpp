#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE direct_path_response_builder_test

#include "asgard/direct_path_response_builder.h"
#include "asgard/request.pb.h"

#include <valhalla/midgard/encoded.h>
#include <valhalla/midgard/pointll.h>
#include <valhalla/proto/trippath.pb.h>
#include <valhalla/thor/pathinfo.h>

#include <boost/test/unit_test.hpp>

using namespace valhalla;

namespace asgard {
namespace direct_path_response_builder {

const std::vector<midgard::PointLL> list_geo_points = {
    {50.12345678f, 1.457634f}, {42.46546513f, 8.4646846f}, {49.1036400f, 3.9731065f}, {42.0794687f, 7.974815640f}};

std::vector<thor::PathInfo> create_path_info_list() {
    std::vector<thor::PathInfo> path_info_list;
    for (size_t i = 0; i < 5; ++i) {
        path_info_list.emplace_back(sif::TravelMode::kDrive, i * 5, baldr::GraphId(), 0);
    }
    return path_info_list;
}

valhalla::odin::TripPath create_trip_path() {
    auto const s = midgard::encode(list_geo_points);

    valhalla::odin::TripPath trip_path;
    trip_path.set_shape(s);
    for (size_t i = 0; i < list_geo_points.size(); ++i) {
        trip_path.add_node()->mutable_edge()->set_length((i * 5) / 1000.f);
    }
    return trip_path;
}

BOOST_AUTO_TEST_CASE(build_journey_response_test) {
    // Empty path_info_list
    {
        pbnavitia::Request request;
        valhalla::odin::TripPath trip_path;
        auto response = build_journey_response(request, {}, trip_path);
        BOOST_CHECK_EQUAL(response.response_type(), pbnavitia::NO_SOLUTION);
        BOOST_CHECK_EQUAL(response.journeys_size(), 0);
    }

    // basic path_info_list (nominal case)
    {
        pbnavitia::Request request;
        std::vector<thor::PathInfo> path_info_list = create_path_info_list();
        request.mutable_direct_path()->set_datetime(1470241573);

        auto trip_path = create_trip_path();
        auto response = build_journey_response(request, path_info_list, trip_path);
        BOOST_CHECK_EQUAL(response.response_type(), pbnavitia::ITINERARY_FOUND);
        BOOST_CHECK_EQUAL(response.journeys_size(), 1);

        auto const journey = response.journeys().begin();
        BOOST_CHECK_EQUAL(journey->nb_sections(), 1);
        BOOST_CHECK_EQUAL(journey->nb_transfers(), 0);
        BOOST_CHECK_EQUAL(journey->duration(), 20);
        BOOST_CHECK_EQUAL(journey->requested_date_time(), 1470241573);
        BOOST_CHECK_EQUAL(journey->departure_date_time(), 1470241573);
        BOOST_CHECK_EQUAL(journey->arrival_date_time(), 1470241593);

        auto const section = journey->sections().begin();
        BOOST_CHECK_EQUAL(section->type(), pbnavitia::STREET_NETWORK);
        BOOST_CHECK_EQUAL(section->id(), "section_0");
        BOOST_CHECK_EQUAL(section->duration(), 20);
        BOOST_CHECK_EQUAL(section->street_network().mode(), pbnavitia::Car);
        BOOST_CHECK_EQUAL(section->begin_date_time(), 1470241573);
        BOOST_CHECK_EQUAL(section->end_date_time(), 1470241593);
        BOOST_CHECK_EQUAL(section->length(), 30);

        auto const origin_coords = section->origin().address().coord();
        BOOST_CHECK_EQUAL(section->origin().uri(), "50.12346;1.45763");
        BOOST_CHECK_EQUAL(section->origin().name(), "");
        BOOST_CHECK_CLOSE(origin_coords.lon(), 50.12345678f, 0.0001f);
        BOOST_CHECK_CLOSE(origin_coords.lat(), 1.457634f, 0.0001f);

        auto const dest_coords = section->destination().address().coord();
        BOOST_CHECK_EQUAL(section->destination().uri(), "42.07947;7.97481");
        BOOST_CHECK_EQUAL(section->destination().name(), "");
        BOOST_CHECK_CLOSE(dest_coords.lon(), 42.0794687f, 0.0001f);
        BOOST_CHECK_CLOSE(dest_coords.lat(), 7.974815640f, 0.0001f);
    }
}

BOOST_AUTO_TEST_CASE(set_extremity_pt_object_test) {
    {
        pbnavitia::Section section;

        set_extremity_pt_object(list_geo_points.front(), section.mutable_origin());
        set_extremity_pt_object(list_geo_points.back(), section.mutable_destination());

        BOOST_CHECK_EQUAL(section.origin().uri(), "50.12346;1.45763");
        BOOST_CHECK_EQUAL(section.origin().name(), "");
        BOOST_CHECK_EQUAL(section.origin().address().coord().lon(), 50.12345678f);
        BOOST_CHECK_EQUAL(section.origin().address().coord().lat(), 1.457634f);

        BOOST_CHECK_EQUAL(section.destination().uri(), "42.07947;7.97482");
        BOOST_CHECK_EQUAL(section.destination().name(), "");
        BOOST_CHECK_EQUAL(section.destination().address().coord().lon(), 42.0794687f);
        BOOST_CHECK_EQUAL(section.destination().address().coord().lat(), 7.974815640f);
    }
}

BOOST_AUTO_TEST_CASE(compute_geojson_test) {
    {
        pbnavitia::Section section;

        compute_geojson(list_geo_points, section);

        BOOST_CHECK_EQUAL(section.mutable_street_network()->coordinates_size(), list_geo_points.size());
        for (size_t i = 0; i < list_geo_points.size(); ++i) {
            const auto coords = section.mutable_street_network()->coordinates(i);
            BOOST_CHECK_EQUAL(coords.lon(), list_geo_points.at(i).lng());
            BOOST_CHECK_EQUAL(coords.lat(), list_geo_points.at(i).lat());
        }
    }
}

void add_section(pbnavitia::Journey& pb_journey, const pbnavitia::SectionType section_type, const pbnavitia::StreetNetworkMode mode,
                 int32_t duration, int32_t length, uint64_t begin_date_time) {
    auto* s = pb_journey.add_sections();
    s->set_type(section_type);
    s->mutable_street_network()->set_mode(mode);
    s->set_id("section");
    s->set_duration(duration);
    s->set_length(length);
    s->set_begin_date_time(begin_date_time);
    s->set_end_date_time(begin_date_time + duration);
}

BOOST_AUTO_TEST_CASE(compute_metadata_test) {
    auto pb_journey = pbnavitia::Journey();
    pb_journey.set_departure_date_time(1470241573);
    pb_journey.set_arrival_date_time(1470242073);

    add_section(pb_journey, pbnavitia::STREET_NETWORK, pbnavitia::StreetNetworkMode::Walking, 1337, 442, 1470241573);
    add_section(pb_journey, pbnavitia::STREET_NETWORK, pbnavitia::StreetNetworkMode::Car, 30, 26, 1470241673);
    add_section(pb_journey, pbnavitia::STREET_NETWORK, pbnavitia::StreetNetworkMode::CarNoPark, 45, 33, 1470241773);
    add_section(pb_journey, pbnavitia::STREET_NETWORK, pbnavitia::StreetNetworkMode::Bike, 62, 777, 1470241873);
    add_section(pb_journey, pbnavitia::STREET_NETWORK, pbnavitia::StreetNetworkMode::Bss, 999, 29, 1470241973);
    add_section(pb_journey, pbnavitia::STREET_NETWORK, pbnavitia::StreetNetworkMode::Ridesharing, 42, 78, 1470242073);

    compute_metadata(pb_journey);

    auto durations = pb_journey.durations();
    auto distances = pb_journey.distances();

    BOOST_CHECK_EQUAL(durations.walking(), 1337);
    BOOST_CHECK_EQUAL(durations.bike(), 1061);
    BOOST_CHECK_EQUAL(durations.car(), 75);
    BOOST_CHECK_EQUAL(durations.ridesharing(), 42);

    // Not the sum of all the lengths
    // arrival_last_section - departure_first_section
    BOOST_CHECK_EQUAL(durations.total(), 542);

    BOOST_CHECK_EQUAL(distances.walking(), 442);
    BOOST_CHECK_EQUAL(distances.bike(), 806);
    BOOST_CHECK_EQUAL(distances.car(), 59);
    BOOST_CHECK_EQUAL(distances.ridesharing(), 78);
}

} // namespace direct_path_response_builder
} // namespace asgard
