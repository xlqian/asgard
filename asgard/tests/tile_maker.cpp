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

#include "tile_maker.h"

#include <valhalla/mjolnir/directededgebuilder.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>

namespace asgard {

namespace tile_maker {

// The graph looks like this
//                           F-->--<--
//                          /        |
//                         /         |
//  A--->--<---B--->--<---C--->--<---D
//              \                    |
//               \                   |
//                E------>--<--------|

void TileMaker::make_tile() {
    using namespace valhalla::mjolnir;
    using namespace valhalla::baldr;

    // make sure that all the old tiles are gone before trying to make new ones.
    if (boost::filesystem::is_directory(tile_dir)) {
        boost::filesystem::remove_all(tile_dir);
    }

    GraphTileBuilder tile(tile_dir, tile_id, false);

    // Set the base lat,lon of the tile
    uint32_t id = tile_id.tileid();
    const auto& tl = TileHierarchy::levels().rbegin();
    PointLL base_ll = tl->tiles.Base(id);
    tile.header_builder().set_base_ll(base_ll);

    uint32_t edge_index = 0;

    auto add_node = [&](const std::pair<GraphId, PointLL>& v, const uint32_t edge_count) {
        NodeInfo node_builder;
        node_builder.set_latlng(base_ll, v.second);
        // node_builder.set_road_class(RoadClass::kSecondary);
        node_builder.set_access(kAllAccess);
        node_builder.set_edge_count(edge_count);
        node_builder.set_edge_index(edge_index);
        node_builder.set_timezone(1);
        edge_index += edge_count;
        tile.nodes().emplace_back(node_builder);
    };

    auto add_edge = [&](const std::pair<GraphId, PointLL>& u,
                        const std::pair<GraphId, PointLL>& v, const uint32_t localedgeidx,
                        const uint32_t opposing_edge_index, const bool forward) {
        DirectedEdgeBuilder edge_builder({}, v.first, forward, u.second.Distance(v.second), 1, 1,
                                         Use::kRoad, RoadClass::kMotorway, localedgeidx,
                                         false, 0, 0, false);
        edge_builder.set_opp_index(opposing_edge_index);
        edge_builder.set_forwardaccess(kAllAccess);
        edge_builder.set_reverseaccess(kAllAccess);
        edge_builder.set_free_flow_speed(100);
        edge_builder.set_constrained_flow_speed(10);
        std::vector<PointLL> shape = {u.second, u.second.PointAlongSegment(v.second), v.second};
        if (!forward) {
            std::reverse(shape.begin(), shape.end());
        }
        bool added;
        // make more complex edge geom so that there are 3 segments, affine combination doesnt properly
        // handle arcs but who cares
        uint32_t edge_info_offset = tile.AddEdgeInfo(localedgeidx, u.first, v.first, 123, // way_id
                                                     0, 0,
                                                     120, // speed limit in kph
                                                     shape,
                                                     {std::to_string(localedgeidx)},
                                                     {std::to_string(localedgeidx)},
                                                     0,
                                                     added);
        // assert(added);
        edge_builder.set_edgeinfo_offset(edge_info_offset);
        tile.directededges().emplace_back(edge_builder);
    };

    add_edge(a, b, 0, 0, true);
    add_node(a, 1);

    add_edge(b, a, 0, 0, false);
    add_edge(b, c, 1, 0, true);
    add_edge(b, e, 3, 0, true);
    add_node(b, 3);

    add_edge(c, b, 1, 1, false);
    add_edge(c, d, 2, 0, true);
    add_edge(c, f, 5, 0, true);
    add_node(c, 3);

    add_edge(d, c, 2, 1, false);
    add_edge(d, e, 4, 1, false);
    add_edge(d, f, 6, 1, false);
    add_node(d, 3);

    add_edge(e, b, 3, 2, false);
    add_edge(e, d, 4, 1, true);
    add_node(e, 2);

    add_edge(f, c, 5, 2, false);
    add_edge(f, d, 6, 2, true);
    add_node(f, 2);

    tile.StoreTileData();

    // write the bin data
    GraphTileBuilder::tweeners_t tweeners;
    auto reloaded = GraphTile::Create(tile_dir, tile_id);
    auto bins = GraphTileBuilder::BinEdges(reloaded, tweeners);
    GraphTileBuilder::AddBins(tile_dir, reloaded, bins);
}

} // namespace tile_maker

} // namespace asgard
