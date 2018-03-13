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

struct Context{
    zmq::context_t& zmq_context;
    valhalla::baldr::GraphReader& graph;
    int max_cache_size;

    Context(zmq::context_t& zmq_context, valhalla::baldr::GraphReader& graph, int max_cache_size) :
            zmq_context(zmq_context), graph(graph), max_cache_size(max_cache_size)
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


void worker(Context& context){
    zmq::context_t& zmq_context =  context.zmq_context;
    valhalla::baldr::GraphReader& graph = context.graph;

    valhalla::thor::TimeDistanceMatrix matrix;
    valhalla::sif::CostFactory<valhalla::sif::DynamicCost> factory;
    factory.Register("auto", valhalla::sif::CreateAutoCost);
    factory.Register("pedestrian", valhalla::sif::CreatePedestrianCost);
    factory.Register("bicycle", valhalla::sif::CreateBicycleCost);

    valhalla::sif::cost_ptr_t mode_costing[static_cast<int>(valhalla::sif::TravelMode::kMaxTravelMode)];
    mode_costing[static_cast<int>(valhalla::sif::TravelMode::kDrive)] = factory.Create("auto", boost::property_tree::ptree());
    mode_costing[static_cast<int>(valhalla::sif::TravelMode::kPedestrian)] = factory.Create("pedestrian", boost::property_tree::ptree());
    mode_costing[static_cast<int>(valhalla::sif::TravelMode::kBicycle)] = factory.Create("bicycle", boost::property_tree::ptree());

    std::map<std::string, valhalla::sif::TravelMode> mode_map;
    mode_map["walking"] = valhalla::sif::TravelMode::kPedestrian;
    mode_map["bike"] = valhalla::sif::TravelMode::kBicycle;
    mode_map["car"] = valhalla::sif::TravelMode::kDrive;

    std::map<std::string, float> mode_distance_map;
    mode_distance_map["walking"] = 60 * 60 * valhalla::thor::kTimeDistCostThresholdPedestrianDivisor;
    mode_distance_map["bike"] = 60 * 60 * valhalla::thor::kTimeDistCostThresholdBicycleDivisor;
    mode_distance_map["car"] = 30 * 60 * valhalla::thor::kTimeDistCostThresholdAutoDivisor;

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
        LOG_INFO("request received");
        pbnavitia::Request pb_req;
        pb_req.ParseFromArray(request.data(), request.size());
        //std::cout << "request received: " << pb_req.DebugString() << std::endl;

        if(pb_req.requested_api() != pbnavitia::street_network_routing_matrix && pb_req.requested_api() != pbnavitia::direct_path){
            //empty response, jormun should be not too sad about it
            pbnavitia::Response response;
            respond(socket, address, response);
            LOG_WARN("wrong request: aborting");
            continue;
        }
        std::cout << pb_req.sn_routing_matrix().origins_size() << "   " << pb_req.sn_routing_matrix().destinations_size() << std::endl;
        std::vector<std::string> sources;
        sources.reserve(pb_req.sn_routing_matrix().origins_size());

        std::vector<std::string> targets;
        targets.reserve(pb_req.sn_routing_matrix().destinations_size());

        std::vector<valhalla::baldr::PathLocation> path_location_sources;
        std::vector<valhalla::baldr::PathLocation> path_location_targets;

        for(const auto& e: pb_req.sn_routing_matrix().origins()){
            sources.push_back(e.place());
        }
        for(const auto& e: pb_req.sn_routing_matrix().destinations()){
            targets.push_back(e.place());
        }

        auto range = boost::range::join(sources, targets);
        LOG_INFO("projecting");
        auto costing = mode_costing[static_cast<int>(mode_map[pb_req.sn_routing_matrix().mode()])];
        auto path_locations = projector(begin(range), end(range), graph, costing);


        for(const auto& e: sources){
            auto it = path_locations.find(e);
            if(it != end(path_locations)){
                path_location_sources.push_back(it->second);
            }else{
                LOG_ERROR("no projection found");
            }
        }

        for(const auto& e: targets){
            auto it = path_locations.find(e);
            if(it != end(path_locations)){
                path_location_targets.push_back(it->second);
            }else{
                LOG_ERROR("no projection found");
            }
        }


        LOG_INFO("matrix");
        std::cout << "navitia mode: " << pb_req.sn_routing_matrix().mode() << " valhalla mode: " << static_cast<int>(mode_map[pb_req.sn_routing_matrix().mode()]) << std::endl;
        auto res = matrix.SourceToTarget(path_location_sources, path_location_targets, graph, mode_costing, mode_map[pb_req.sn_routing_matrix().mode()], mode_distance_map[pb_req.sn_routing_matrix().mode()]);
        LOG_INFO("matrixed!!!");

        //todo we need to look with sources and targets since path_location only containts coord correctly projected
        auto it = begin(res);
        pbnavitia::Response response;
        int nb_reached = 0;
        //in fact jormun don't want a real matrix, only a vector of solution :(
        auto* row = response.mutable_sn_routing_matrix()->add_rows();
        for(const auto& source: path_location_sources){
            for(const auto& target: path_location_targets){
                auto* k = row->add_routing_response();
                k->set_duration(it->time);
                if(it->time == valhalla::thor::kMaxCost){
                    k->set_routing_status(pbnavitia::RoutingStatus::unknown);
                }else{
                    k->set_routing_status(pbnavitia::RoutingStatus::reached);
                    ++nb_reached;
                }
                ++it;
            }
        }

        std::cout << "nb result: " << res.size() << std::endl;
        std::cout << "nb reached: " << nb_reached << std::endl;
        /*
        for(auto r: res){
            std::cout << "time: " << r.time << " distance: " << r.dist << std::endl;
        }
        */
        //std::cout << "response: " << response.DebugString() << std::endl;

        respond(socket, address, response);
        LOG_INFO("respond sent");
        if(graph.OverCommitted()){
            graph.Clear();
        }

        LOG_INFO("cleared!!!");
    }
}


template<typename T>
const T get_config(const std::string& key, const T& default_value=T()){
    char* v = std::getenv(key.c_str());
    if(v != nullptr){
        return boost::lexical_cast<T>(v);
    }
    return default_value;
}

int main(){

    const std::string socket_path = get_config<std::string>("ASGARD_SOCKET_PATH", "tcp://*:6000");
    const std::string tile_extract = get_config<std::string>("ASGARD_TILE_EXTRACT", "/data/valhalla/tiles.tar");
    const std::string tile_dir = get_config<std::string>("ASGARD_TILE_DIR", "/data/valhalla/tiles");
    const int cache_size = get_config<int>("ASGARD_CACHE_SIZE", 100000);

    boost::thread_group threads;
    zmq::context_t context(1);
    LoadBalancer lb(context);
    lb.bind(socket_path, "inproc://workers");

    boost::property_tree::ptree ptree;
    ptree.put("max_cache_size", 1000000000);
    ptree.put("tile_dir", tile_dir);
    ptree.put("tile_extract", tile_extract);

    valhalla::baldr::GraphReader graph(ptree);


    for(int thread_nbr = 0; thread_nbr < 3; ++thread_nbr) {
        threads.create_thread(std::bind(&worker, Context(context, graph, cache_size)));
    }

    // Connect worker threads to client threads via a queue
    do{
        try{
            lb.run();
        }catch(const zmq::error_t&){}//lors d'un SIGHUP on restore la queue
    }while(true);




}
