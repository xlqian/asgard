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

#include "asgard/context.h"
#include "asgard/projector.h"
#include "asgard/request.pb.h"
#include "asgard/response.pb.h"

#include <valhalla/baldr/graphreader.h>
#include <valhalla/sif/costfactory.h>
#include <valhalla/sif/costconstants.h>
#include <valhalla/thor/timedistancematrix.h>
#include <valhalla/thor/bidirectional_astar.h>

namespace asgard {

static const size_t mode_costing_size = static_cast<size_t>(valhalla::sif::TravelMode::kMaxTravelMode);
using ModeCosting = valhalla::sif::cost_ptr_t[mode_costing_size];

struct Handler {
    Handler(const Context&);
    pbnavitia::Response handle(const pbnavitia::Request&);
private:
    pbnavitia::Response handle_matrix(const pbnavitia::Request&);
    pbnavitia::Response handle_direct_path(const pbnavitia::Request&);
    pbnavitia::Response build_journey_response(const pbnavitia::Request&, const std::vector<valhalla::thor::PathInfo>&);

    valhalla::baldr::GraphReader graph;
    valhalla::thor::TimeDistanceMatrix matrix;
    valhalla::thor::BidirectionalAStar bda;
    valhalla::sif::CostFactory<valhalla::sif::DynamicCost> factory;
    ModeCosting mode_costing;
    asgard::Projector projector;
};

}
