#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE projector_test

#include "asgard/mode_costing.h"
#include "asgard/projector.h"
#include "asgard/util.h"

#include <valhalla/midgard/pointll.h>
#include <valhalla/mjolnir/directededgebuilder.h>
#include <valhalla/mjolnir/graphtilebuilder.h>
#include <valhalla/sif/costconstants.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/test/unit_test.hpp>

#if !defined(TESTS_SOURCE_DIR)
#define TESTS_SOURCE_DIR
#endif

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

auto tile_dir = std::string(TESTS_SOURCE_DIR) + "tile_dir";
GraphId tile_id = TileHierarchy::GetGraphId({.125, .125}, 2);
PointLL base_ll = TileHierarchy::get_tiling(tile_id.level()).Base(tile_id.tileid());
std::pair<GraphId, PointLL> b({tile_id.tileid(), tile_id.level(), 0}, {.01, .2});
std::pair<GraphId, PointLL> a({tile_id.tileid(), tile_id.level(), 1}, {.01, .1});
std::pair<GraphId, PointLL> c({tile_id.tileid(), tile_id.level(), 2}, {.01, .01});
std::pair<GraphId, PointLL> d({tile_id.tileid(), tile_id.level(), 3}, {.2, .1});

void make_tile() {
    using namespace valhalla::mjolnir;
    using namespace valhalla::baldr;

    // make sure that all the old tiles are gone before trying to make new ones.
    if (boost::filesystem::is_directory(tile_dir)) {
        boost::filesystem::remove_all(tile_dir);
    }

    // basic tile information
    GraphTileBuilder tile(tile_dir, tile_id, false);
    uint32_t edge_index = 0;

    auto add_node = [&edge_index](const std::pair<GraphId, PointLL>& v, const uint32_t edge_count) {
        NodeInfo node_builder;
        node_builder.set_latlng(base_ll, v.second);
        node_builder.set_edge_count(edge_count);
        node_builder.set_edge_index(edge_index);
        edge_index += edge_count;
        return node_builder;
    };
    auto add_edge = [&tile](const std::pair<GraphId, PointLL>& u, const std::pair<GraphId, PointLL>& v,
                            const uint32_t name, const uint32_t opposing, const bool forward) {
        DirectedEdgeBuilder edge_builder({}, v.first, forward, u.second.Distance(v.second) + .5, 1, 1, {},
                                         {}, 0, false, 0, 0);
        edge_builder.set_opp_index(opposing);
        edge_builder.set_forwardaccess(kAllAccess);
        std::vector<PointLL> shape = {u.second, u.second.MidPoint(v.second), v.second};
        if (!forward)
            std::reverse(shape.begin(), shape.end());

        bool add;
        // make more complex edge geom so that there are 3 segments, affine combination doesnt properly
        // handle arcs but who cares
        uint32_t edge_info_offset = tile.AddEdgeInfo(name, u.first, v.first, 123, 456, 0, 55, shape,
                                                     {std::to_string(name)}, 0, add);
        edge_builder.set_edgeinfo_offset(edge_info_offset);
        return edge_builder;
    };

    // B
    {
        tile.directededges().emplace_back(add_edge(b, d, 0, 0, false)); // 0
        tile.directededges().emplace_back(add_edge(b, a, 2, 0, true));  // 1
        tile.nodes().emplace_back(add_node(b, 2));
    }

    // A
    {
        tile.directededges().emplace_back(add_edge(a, b, 2, 1, false)); // 2
        tile.directededges().emplace_back(add_edge(a, d, 3, 1, true));  // 3
        tile.directededges().emplace_back(add_edge(a, c, 1, 0, false)); // 4
        tile.nodes().emplace_back(add_node(a, 3));
    }

    // C
    {
        tile.directededges().emplace_back(add_edge(c, a, 1, 2, true));  // 5
        tile.directededges().emplace_back(add_edge(c, d, 4, 2, false)); // 6
        tile.nodes().emplace_back(add_node(c, 2));
    }

    // D
    {
        tile.directededges().emplace_back(add_edge(d, b, 0, 0, true));  // 7
        tile.directededges().emplace_back(add_edge(d, a, 3, 1, false)); // 8
        tile.directededges().emplace_back(add_edge(d, c, 4, 1, true));  // 9
        tile.nodes().emplace_back(add_node(d, 3));
    }

    // write the tile
    tile.StoreTileData();

    // write the bin data
    GraphTileBuilder::tweeners_t tweeners;
    GraphTile reloaded(tile_dir, tile_id);
    auto bins = GraphTileBuilder::BinEdges(&reloaded, tweeners);
    GraphTileBuilder::AddBins(tile_dir, &reloaded, bins);
}

BOOST_AUTO_TEST_CASE(simple_projector_test) {
    make_tile();
    boost::property_tree::ptree conf;
    conf.put("tile_dir", tile_dir);
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
        std::vector<std::string> locations = {"coord:.01:.2"};
        auto result = p(begin(locations), end(locations), graph, "car", costing);
        BOOST_CHECK_EQUAL(result.size(), 1);
        BOOST_CHECK_EQUAL(p.get_nb_cache_miss(), 2);
        BOOST_CHECK_EQUAL(p.get_nb_calls(), 2);
    }
    // cache = { coord:.01:.2 }
    {
        std::vector<std::string> locations = {"coord:.01:.2"};
        auto result = p(begin(locations), end(locations), graph, "car", costing);
        BOOST_CHECK_EQUAL(result.size(), 1);
        BOOST_CHECK_EQUAL(p.get_nb_cache_miss(), 2);
        BOOST_CHECK_EQUAL(p.get_nb_calls(), 3);
    }
    // cache = { coord:.01:.2 }
    {
        std::vector<std::string> locations = {"coord:.01:.1"};
        auto result = p(begin(locations), end(locations), graph, "car", costing);
        BOOST_CHECK_EQUAL(result.size(), 1);
        BOOST_CHECK_EQUAL(p.get_nb_cache_miss(), 3);
        BOOST_CHECK_EQUAL(p.get_nb_calls(), 4);
    }
    // cache = { coord:.01:.1; coord:.01:.2 }
    {
        std::vector<std::string> locations = {"coord:.01:.2"};
        auto result = p(begin(locations), end(locations), graph, "car", costing);
        BOOST_CHECK_EQUAL(result.size(), 1);
        BOOST_CHECK_EQUAL(p.get_nb_cache_miss(), 3);
        BOOST_CHECK_EQUAL(p.get_nb_calls(), 5);
    }
    // cache = { coord:.01:.2; coord:.01:.1 }
    {
        std::vector<std::string> locations = {"coord:.2:.1"};
        auto result = p(begin(locations), end(locations), graph, "car", costing);
        BOOST_CHECK_EQUAL(result.size(), 1);
        BOOST_CHECK_EQUAL(p.get_nb_cache_miss(), 4);
        BOOST_CHECK_EQUAL(p.get_nb_calls(), 6);
    }
    // cache = { coord:.2:.1; coord:.01:.2 }
    {
        std::vector<std::string> locations = {"coord:.01:.1"};
        auto result = p(begin(locations), end(locations), graph, "car", costing);
        BOOST_CHECK_EQUAL(result.size(), 1);
        BOOST_CHECK_EQUAL(p.get_nb_cache_miss(), 5);
        BOOST_CHECK_EQUAL(p.get_nb_calls(), 7);
    }
    // cache = { coord:.01:.1; coord:.2:.1 }
}

BOOST_AUTO_TEST_CASE(build_location_test) {
    UnitTestProjector testProjector(3);
    {
        BOOST_CHECK_THROW(testProjector.build_location("plop", 0, 0), navitia::wrong_coordinate);
    }
    {
        const auto l = testProjector.build_location("coord:.01:.1", 12u, 42l);
        BOOST_CHECK_CLOSE(l.latlng_.lng(), .01f, .0001f);
        BOOST_CHECK_CLOSE(l.latlng_.lat(), .1f, .0001f);
        BOOST_CHECK_EQUAL(static_cast<bool>(l.stoptype_), false);
        BOOST_CHECK_EQUAL(l.minimum_reachability_, 12u);
        BOOST_CHECK_EQUAL(l.radius_, 42l);
    }
    {
        const auto l = testProjector.build_location("coord:.01:.1", 12u, 42l);
        BOOST_CHECK_CLOSE(l.latlng_.lng(), .01f, .0001f);
        BOOST_CHECK_CLOSE(l.latlng_.lat(), .1f, .0001f);
        BOOST_CHECK_EQUAL(static_cast<bool>(l.stoptype_), false);
        BOOST_CHECK_EQUAL(l.minimum_reachability_, 12u);
        BOOST_CHECK_EQUAL(l.radius_, 42l);
        BOOST_CHECK_EQUAL(l.name_, "coord:.01:.1");
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
