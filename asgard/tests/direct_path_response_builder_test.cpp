#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE direct_path_response_builder_test

#include <boost/test/unit_test.hpp>

#include "asgard/direct_path_response_builder.h"


using namespace valhalla;
namespace adprb = asgard::direct_path_response_builder;

static std::vector<thor::PathInfo> create_path_info_list() {
    std::vector<thor::PathInfo> path_info_list;
    for (auto i = 0; i < 5; ++i) {
        path_info_list.emplace_back(sif::TravelMode::kDrive, i * 5, baldr::GraphId(), 0);

    }
    return path_info_list;
}

BOOST_AUTO_TEST_CASE(build_journey_response_test) {
    // Empty path_info_list
    {
        pbnavitia::Request request;
        auto response = adprb::build_journey_response(request, {}, 0.f);
        BOOST_CHECK_EQUAL(response.response_type(), pbnavitia::NO_SOLUTION);
        BOOST_CHECK_EQUAL(response.journeys_size(), 0);
    }

    // basic path_info_list (nominal case)
    {
        pbnavitia::Request request;
        std::vector<thor::PathInfo> path_info_list = create_path_info_list();
        request.mutable_direct_path()->set_datetime(1470241573);

        auto response = adprb::build_journey_response(request, path_info_list, 42666);
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
        BOOST_CHECK_EQUAL(section->id(), "section");
        BOOST_CHECK_EQUAL(section->duration(), 20);
        BOOST_CHECK_EQUAL(section->street_network().mode(), pbnavitia::Car);
        BOOST_CHECK_EQUAL(section->begin_date_time(), 1470241573);
        BOOST_CHECK_EQUAL(section->end_date_time(), 1470241593);
        BOOST_CHECK_EQUAL(section->length(), 42666);
    }
}

static void add_section(pbnavitia::Journey& pb_journey, const pbnavitia::SectionType section_type, const pbnavitia::StreetNetworkMode mode,
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

    adprb::compute_metadata(pb_journey);

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

