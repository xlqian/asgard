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

#include "projector.h"

#include "utils/zmq.h"
#include "asgard/request.pb.h"
#include "asgard/response.pb.h"

#include <valhalla/loki/search.h>
#include <valhalla/baldr/graphreader.h>
#include <valhalla/baldr/location.h>
#include <valhalla/thor/timedistancematrix.h>
#include <valhalla/sif/costfactory.h>
#include <valhalla/sif/costconstants.h>
#include <valhalla/midgard/logging.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/format.hpp>
#include <boost/range/join.hpp>

namespace pt = boost::property_tree;

struct Context{
    zmq::context_t& zmq_context;
    pt::ptree ptree;
    int max_cache_size;

    Context(zmq::context_t& zmq_context, pt::ptree ptree, int max_cache_size) :
        zmq_context(zmq_context), ptree(ptree), max_cache_size(max_cache_size)
    {}

};


static void respond(zmq::socket_t& socket,
             const std::string& address,
             const pbnavitia::Response& response){
    zmq::message_t reply(response.ByteSize());
    try{
        response.SerializeToArray(reply.data(), response.ByteSize());
    }catch(const google::protobuf::FatalException& e){
        pbnavitia::Response error_response;
        error_response.mutable_error()->set_id(pbnavitia::Error::internal_error);
        error_response.mutable_error()->set_message(e.what());
        reply.rebuild(error_response.ByteSize());
        error_response.SerializeToArray(reply.data(), error_response.ByteSize());
    }
    z_send(socket, address, ZMQ_SNDMORE);
    z_send(socket, "", ZMQ_SNDMORE);
    socket.send(reply);
}

static const std::map<std::string, valhalla::sif::TravelMode> mode_map = {
    {"walking", valhalla::sif::TravelMode::kPedestrian},
    {"bike", valhalla::sif::TravelMode::kBicycle},
    {"car", valhalla::sif::TravelMode::kDrive},
};
static const size_t mode_costing_size = static_cast<size_t>(valhalla::sif::TravelMode::kMaxTravelMode);
using ModeCosting = valhalla::sif::cost_ptr_t[mode_costing_size];

static size_t mode_index(const std::string& mode) {
    return static_cast<size_t>(mode_map.at(mode));
}

static pt::ptree make_costing_option(const std::string& mode, float speed) {
    pt::ptree ptree;
    speed *= 3.6;
    if (mode == "walking") {
        ptree.put("walking_speed", speed);
    } else if (mode == "bike") {
        ptree.put("cycling_speed", speed);
        ptree.put("bicycle_type", "Hybrid");
    }
    return ptree;
}

void worker(Context& context){
    zmq::context_t& zmq_context =  context.zmq_context;

    valhalla::baldr::GraphReader graph(context.ptree.get_child("mjolnir"));

    valhalla::thor::TimeDistanceMatrix matrix;
    valhalla::sif::CostFactory<valhalla::sif::DynamicCost> factory;
    factory.Register("car", valhalla::sif::CreateAutoCost);
    factory.Register("walking", valhalla::sif::CreatePedestrianCost);
    factory.Register("bike", valhalla::sif::CreateBicycleCost);

    ModeCosting mode_costing = {};
    mode_costing[mode_index("car")] = factory.Create("car", pt::ptree());
    mode_costing[mode_index("walking")] = factory.Create("walking", pt::ptree());
    mode_costing[mode_index("bike")] = factory.Create("bike", pt::ptree());

    const std::map<std::string, float> mode_distance_map = {
        {"walking", context.ptree.get<float>("service_limits.pedestrian.max_matrix_distance")},
        {"bike", context.ptree.get<float>("service_limits.bicycle.max_matrix_distance")},
        {"car", context.ptree.get<float>("service_limits.auto.max_matrix_distance")},
    };

    asgard::Projector projector(context.max_cache_size);

    zmq::socket_t socket (zmq_context, ZMQ_REQ);
    socket.connect("inproc://workers");
    z_send(socket, "READY");

    while(1){

        const std::string address = z_recv(socket);
        {
            std::string empty = z_recv(socket);
            assert(empty.size() == 0);
        }

        zmq::message_t request;
        socket.recv(&request);
        pbnavitia::Request pb_req;
        pb_req.ParseFromArray(request.data(), request.size());
        LOG_INFO("Request received...");

        if(pb_req.requested_api() != pbnavitia::street_network_routing_matrix && pb_req.requested_api() != pbnavitia::direct_path){
            //empty response, jormun should be not too sad about it
            pbnavitia::Response response;
            respond(socket, address, response);
            LOG_WARN("wrong request: aborting");
            continue;
        }
        LOG_INFO("Processing matrix request " +
                 std::to_string(pb_req.sn_routing_matrix().origins_size()) + "x" +
                 std::to_string(pb_req.sn_routing_matrix().destinations_size()));
        const std::string mode = pb_req.sn_routing_matrix().mode();
        std::vector<std::string> sources;
        sources.reserve(pb_req.sn_routing_matrix().origins_size());

        std::vector<std::string> targets;
        targets.reserve(pb_req.sn_routing_matrix().destinations_size());

        for(const auto& e: pb_req.sn_routing_matrix().origins()){
            sources.push_back(e.place());
        }
        for(const auto& e: pb_req.sn_routing_matrix().destinations()){
            targets.push_back(e.place());
        }

        mode_costing[mode_index(mode)] = factory.Create(mode, make_costing_option(mode, pb_req.sn_routing_matrix().speed()));
        auto costing = mode_costing[mode_index(mode)];

        LOG_INFO("Projecting " + std::to_string(sources.size() + targets.size()) + " locations...");
        auto range = boost::range::join(sources, targets);
        auto path_locations = projector(begin(range), end(range), graph, costing);
        LOG_INFO("Projecting locations done.");

        google::protobuf::RepeatedPtrField<valhalla::odin::Location> path_location_sources;
        google::protobuf::RepeatedPtrField<valhalla::odin::Location> path_location_targets;
        for (const auto& e: sources) {
            valhalla::baldr::PathLocation::toPBF(path_locations.at(e), path_location_sources.Add(), graph);
        }
        for (const auto& e: targets) {
            valhalla::baldr::PathLocation::toPBF(path_locations.at(e), path_location_targets.Add(), graph);
        }

        LOG_INFO("Computing matrix...");
        auto res = matrix.SourceToTarget(path_location_sources,
                                         path_location_targets,
                                         graph,
                                         mode_costing,
                                         mode_map.at(mode),
                                         mode_distance_map.at(mode));
        LOG_INFO("Computing matrix done.");

        pbnavitia::Response response;
        int nb_unknown = 0;
        int nb_unreached = 0;
        //in fact jormun don't want a real matrix, only a vector of solution :(
        auto* row = response.mutable_sn_routing_matrix()->add_rows();
        assert(res.size() == source.size() * target.size());
        for (const auto& elt: res) {
            auto* k = row->add_routing_response();
            k->set_duration(elt.time);
            if (elt.time == valhalla::thor::kMaxCost) {
                k->set_routing_status(pbnavitia::RoutingStatus::unknown);
                ++nb_unknown;
            } else if (elt.time > uint32_t(pb_req.sn_routing_matrix().max_duration())) {
                k->set_routing_status(pbnavitia::RoutingStatus::unreached);
                ++nb_unreached;
            } else {
                k->set_routing_status(pbnavitia::RoutingStatus::reached);
            }
        }

        respond(socket, address, response);
        LOG_INFO("Request done with " +
                 std::to_string(nb_unknown) + " unknown and " +
                 std::to_string(nb_unreached) + " unreached");

        if (graph.OverCommitted()) { graph.Clear(); }
        LOG_INFO("Everything is clear.");
    }
}


template<typename T>
const T get_config(const std::string& key, T value=T()){
    char* v = std::getenv(key.c_str());
    if (v != nullptr) {
        value = boost::lexical_cast<T>(v);
    }
    LOG_INFO("Config: " + key + "=" + boost::lexical_cast<std::string>(value));
    return value;
}

int main(){
    const auto socket_path = get_config<std::string>("ASGARD_SOCKET_PATH", "tcp://*:6000");
    const auto cache_size = get_config<size_t>("ASGARD_CACHE_SIZE", 100000);
    const auto valhalla_conf = get_config<std::string>("ASGARD_VALHALLA_CONF", "/data/valhalla/valhalla.conf");

    boost::thread_group threads;
    zmq::context_t context(1);
    LoadBalancer lb(context);
    lb.bind(socket_path, "inproc://workers");

    pt::ptree ptree;
    pt::read_json(valhalla_conf, ptree);

    for(int thread_nbr = 0; thread_nbr < 3; ++thread_nbr) {
        threads.create_thread(std::bind(&worker, Context(context, ptree, cache_size)));
    }

    // Connect worker threads to client threads via a queue
    do{
        try{
            lb.run();
        }catch(const zmq::error_t&){}//lors d'un SIGHUP on restore la queue
    }while(true);




}
