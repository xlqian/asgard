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

#include "utils/zmq.h"
#include <boost/property_tree/ptree.hpp>

namespace asgard {

struct Context {
    zmq::context_t& zmq_context;
    boost::property_tree::ptree ptree;
    int max_cache_size;

    Context(zmq::context_t& zmq_context, boost::property_tree::ptree ptree, int max_cache_size) :
        zmq_context(zmq_context), ptree(ptree), max_cache_size(max_cache_size)
    {}
};

}
