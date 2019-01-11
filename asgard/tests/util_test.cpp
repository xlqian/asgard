#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE util_test

#include "asgard/util.h"
#include <valhalla/sif/costconstants.h>
#include <boost/test/unit_test.hpp>

using namespace valhalla;

namespace asgard {

namespace util {

BOOST_AUTO_TEST_CASE(convert_valhalla_to_navitia_mode_test) {
    BOOST_CHECK_EQUAL(convert_valhalla_to_navitia_mode(sif::TravelMode::kDrive), pbnavitia::Car);
    BOOST_CHECK_EQUAL(convert_valhalla_to_navitia_mode(sif::TravelMode::kPedestrian), pbnavitia::Walking);
    BOOST_CHECK_EQUAL(convert_valhalla_to_navitia_mode(sif::TravelMode::kBicycle), pbnavitia::Bike);
    BOOST_CHECK_THROW(convert_valhalla_to_navitia_mode(sif::TravelMode::kPublicTransit), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(convert_navitia_to_valhalla_mode_test) {
    BOOST_CHECK_EQUAL(static_cast<u_int8_t>(convert_navitia_to_valhalla_mode("walking")), 1U);
    BOOST_CHECK_EQUAL(static_cast<u_int8_t>(convert_navitia_to_valhalla_mode("bike")), 2U);
    BOOST_CHECK_EQUAL(static_cast<u_int8_t>(convert_navitia_to_valhalla_mode("car")), 0U);
    BOOST_CHECK_THROW(convert_navitia_to_valhalla_costing("plopi"), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(navitia_to_valhalla_mode_index_test) {
    BOOST_CHECK_EQUAL(navitia_to_valhalla_mode_index("walking"), size_t(1));
    BOOST_CHECK_EQUAL(navitia_to_valhalla_mode_index("bike"), size_t(2));
    BOOST_CHECK_EQUAL(navitia_to_valhalla_mode_index("car"), size_t(0));
    BOOST_CHECK_THROW(convert_navitia_to_valhalla_costing("plopi"), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(convert_navitia_to_valhalla_costing_test) {
    BOOST_CHECK_EQUAL(convert_navitia_to_valhalla_costing("walking"), odin::Costing::pedestrian);
    BOOST_CHECK_EQUAL(convert_navitia_to_valhalla_costing("bike"), odin::Costing::bicycle);
    BOOST_CHECK_EQUAL(convert_navitia_to_valhalla_costing("car"), odin::Costing::auto_);
    BOOST_CHECK_THROW(convert_navitia_to_valhalla_costing("plopi"), std::out_of_range);
}

} // namespace util

} // namespace asgard
