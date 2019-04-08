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

#include <valhalla/baldr/graphreader.h>

#include <boost/property_tree/ptree.hpp>

namespace zmq {
class context_t;
}

namespace asgard {

class Metrics;
class Projector;

struct Context {
    zmq::context_t& zmq_context;
    valhalla::baldr::GraphReader& graph;
    const Metrics& metrics;
    const Projector& projector;

    Context(zmq::context_t& zmq_context, valhalla::baldr::GraphReader& graph,
            const Metrics& metrics, const Projector& projector) : zmq_context(zmq_context),
                                                                  graph(graph),
                                                                  metrics(metrics),
                                                                  projector(projector) {}
};

} // namespace asgard
