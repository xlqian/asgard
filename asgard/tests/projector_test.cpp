#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE projector_test

#include "tile_maker.h"

#include "asgard/mode_costing.h"
#include "asgard/projector.h"
#include "asgard/util.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/test/unit_test.hpp>

using namespace valhalla;

namespace asgard {

class UnitTestProjector {
public:
    UnitTestProjector(size_t cache_size = 5,
                      unsigned int reachability = 0,
                      unsigned int radius = 0) : p(cache_size, reachability, radius) {}

    valhalla::baldr::Location build_location(const std::string& place,
                                             unsigned int reachability,
                                             unsigned int radius) const {
        return p.build_location(place, reachability, radius);
    }

private:
    Projector p;
};

BOOST_AUTO_TEST_CASE(simple_projector_test) {
    tile_maker::TileMaker maker;
    maker.make_tile();

    boost::property_tree::ptree conf;
    conf.put("tile_dir", maker.get_tile_dir());
    valhalla::baldr::GraphReader graph(conf);

    ModeCosting mode_costing;
    auto costing = mode_costing.get_costing_for_mode("car");
    Projector p(2);
    // cache = {}
    {
        std::vector<std::string> locations = {"coord:2:2"};
        auto result = p(begin(locations), end(locations), graph, "car", costing);
        BOOST_CHECK_EQUAL(result.size(), 0);
        BOOST_CHECK_EQUAL(p.get_nb_cache_miss(), 1);
        BOOST_CHECK_EQUAL(p.get_nb_calls(), 1);
    }
    // cache = {}
    {
        std::vector<std::string> locations = {"coord:.03:.01"};
        auto result = p(begin(locations), end(locations), graph, "car", costing);
        BOOST_CHECK_EQUAL(result.size(), 1);
        BOOST_CHECK_EQUAL(p.get_nb_cache_miss(), 2);
        BOOST_CHECK_EQUAL(p.get_nb_calls(), 2);
    }
    // cache = { coord:.03:.01 }
    {
        std::vector<std::string> locations = {"coord:.03:.01"};
        auto result = p(begin(locations), end(locations), graph, "car", costing);
        BOOST_CHECK_EQUAL(result.size(), 1);
        BOOST_CHECK_EQUAL(p.get_nb_cache_miss(), 2);
        BOOST_CHECK_EQUAL(p.get_nb_calls(), 3);
    }
    // cache = { coord:.03:.01 }
    {
        std::vector<std::string> locations = {"coord:.09:.01"};
        auto result = p(begin(locations), end(locations), graph, "car", costing);
        BOOST_CHECK_EQUAL(result.size(), 1);
        BOOST_CHECK_EQUAL(p.get_nb_cache_miss(), 3);
        BOOST_CHECK_EQUAL(p.get_nb_calls(), 4);
    }
    // cache = { coord:.09:.01; coord:.03:.01 }
    {
        std::vector<std::string> locations = {"coord:.03:.01"};
        auto result = p(begin(locations), end(locations), graph, "car", costing);
        BOOST_CHECK_EQUAL(result.size(), 1);
        BOOST_CHECK_EQUAL(p.get_nb_cache_miss(), 3);
        BOOST_CHECK_EQUAL(p.get_nb_calls(), 5);
    }
    // cache = { coord:.03:.01; coord:.09:.01 }
    {
        std::vector<std::string> locations = {"coord:.13:.01"};
        auto result = p(begin(locations), end(locations), graph, "car", costing);
        BOOST_CHECK_EQUAL(result.size(), 1);
        BOOST_CHECK_EQUAL(p.get_nb_cache_miss(), 4);
        BOOST_CHECK_EQUAL(p.get_nb_calls(), 6);
    }
    // cache = { coord:.13:.01; coord:.03:.01 }
    {
        std::vector<std::string> locations = {"coord:.09:.01"};
        auto result = p(begin(locations), end(locations), graph, "car", costing);
        BOOST_CHECK_EQUAL(result.size(), 1);
        BOOST_CHECK_EQUAL(p.get_nb_cache_miss(), 5);
        BOOST_CHECK_EQUAL(p.get_nb_calls(), 7);
    }
    // cache = { coord:.09:.01; coord:.13:.01 }
}

BOOST_AUTO_TEST_CASE(build_location_test) {
    UnitTestProjector testProjector(3);
    {
        BOOST_CHECK_THROW(testProjector.build_location("plop", 0, 0), navitia::wrong_coordinate);
    }
    {
        const auto l = testProjector.build_location("coord:8:0", 12u, 42l);
        BOOST_CHECK_CLOSE(l.latlng_.lng(), 8.f, .0001f);
        BOOST_CHECK_CLOSE(l.latlng_.lat(), 0.f, .0001f);
        BOOST_CHECK_EQUAL(static_cast<bool>(l.stoptype_), false);
        BOOST_CHECK_EQUAL(l.minimum_reachability_, 12u);
        BOOST_CHECK_EQUAL(l.radius_, 42l);
    }
    {
        const auto l = testProjector.build_location("coord:8:0", 12u, 42l);
        BOOST_CHECK_CLOSE(l.latlng_.lng(), 8.f, .0001f);
        BOOST_CHECK_CLOSE(l.latlng_.lat(), 0.f, .0001f);
        BOOST_CHECK_EQUAL(static_cast<bool>(l.stoptype_), false);
        BOOST_CHECK_EQUAL(l.minimum_reachability_, 12u);
        BOOST_CHECK_EQUAL(l.radius_, 42l);
        BOOST_CHECK_EQUAL(l.name_, "coord:8:0");
    }
    {
        const auto l = testProjector.build_location("92;43", 29u, 15l);
        BOOST_CHECK_CLOSE(l.latlng_.lng(), 92.f, .0001f);
        BOOST_CHECK_CLOSE(l.latlng_.lat(), 43.f, .0001f);
        BOOST_CHECK_EQUAL(static_cast<bool>(l.stoptype_), false);
        BOOST_CHECK_EQUAL(l.minimum_reachability_, 29u);
        BOOST_CHECK_EQUAL(l.radius_, 15l);
        BOOST_CHECK_EQUAL(l.name_, "92;43");
    }
}

} // namespace asgard
