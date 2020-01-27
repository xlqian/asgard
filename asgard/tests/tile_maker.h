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

#pragma once

#include <valhalla/baldr/tilehierarchy.h>
#include <valhalla/mjolnir/graphtilebuilder.h>

#include <string>

#if !defined(TESTS_BUILD_DIR)
#define TESTS_BUILD_DIR
#endif

namespace asgard {

namespace tile_maker {

class TileMaker {
public:
    TileMaker() = default;

    // Create tile for unit tests
    void make_tile();

    const std::string& get_tile_dir() const { return tile_dir; }
    const std::vector<PointLL>& get_all_points() const { return all_points; }

private:
    const std::string tile_dir = std::string(TESTS_BUILD_DIR) + "tile_dir";

    const GraphId tile_id = TileHierarchy::GetGraphId({.125, .125}, 2);
    const PointLL base_ll = TileHierarchy::get_tiling(tile_id.level()).Base(tile_id.tileid());

    const std::pair<GraphId, PointLL> a = std::pair<GraphId, PointLL>({tile_id.tileid(), tile_id.level(), 0}, {.001, .003});
    const std::pair<GraphId, PointLL> b = std::pair<GraphId, PointLL>({tile_id.tileid(), tile_id.level(), 1}, {.003, .003});
    const std::pair<GraphId, PointLL> c = std::pair<GraphId, PointLL>({tile_id.tileid(), tile_id.level(), 2}, {.009, .003});
    const std::pair<GraphId, PointLL> d = std::pair<GraphId, PointLL>({tile_id.tileid(), tile_id.level(), 3}, {.013, .003});
    const std::pair<GraphId, PointLL> e = std::pair<GraphId, PointLL>({tile_id.tileid(), tile_id.level(), 4}, {.007, .001});
    const std::pair<GraphId, PointLL> f = std::pair<GraphId, PointLL>({tile_id.tileid(), tile_id.level(), 5}, {.010, .005});

    const std::vector<PointLL> all_points = {a.second, b.second, c.second, d.second, e.second, f.second};
};
} // namespace tile_maker

} // namespace asgard
