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

#include "context.h"
#include "handler.h"
#include "utils/zmq.h"

#include "asgard/metrics.h"
#include "asgard/request.pb.h"
#include "asgard/projector.h"

#include <valhalla/midgard/logging.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/thread.hpp>

using namespace valhalla;

static void respond(zmq::socket_t& socket,
                    const std::string& address,
                    const pbnavitia::Response& response) {
    zmq::message_t reply(response.ByteSize());
    try {
        response.SerializeToArray(reply.data(), response.ByteSize());
    } catch (const google::protobuf::FatalException& e) {
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

namespace ptime = boost::posix_time;
static void worker(const asgard::Context& context) {
    zmq::context_t& zmq_context = context.zmq_context;
    asgard::Handler handler(context);

    zmq::socket_t socket(zmq_context, ZMQ_REQ);
    socket.connect("inproc://workers");
    z_send(socket, "READY");

    while (1) {

        const std::string address = z_recv(socket);
        {
            std::string empty = z_recv(socket);
            assert(empty.size() == 0);
        }

        zmq::message_t request;
        socket.recv(&request);
        asgard::InFlightGuard in_flight_guard(context.metrics.start_in_flight());
        pbnavitia::Request pb_req;
        pb_req.ParseFromArray(request.data(), request.size());
        LOG_INFO("Request received...");

        const auto response = handler.handle(pb_req);

        respond(socket, address, response);
    }
}

template<typename T>
const T get_config(const std::string& key, T value = T()) {
    char* v = std::getenv(key.c_str());
    if (v != nullptr) {
        value = boost::lexical_cast<T>(v);
    }
    LOG_INFO("Config: " + key + "=" + boost::lexical_cast<std::string>(value));
    return value;
}

namespace ptree = boost::property_tree;
int main() {
    const auto socket_path = get_config<std::string>("ASGARD_SOCKET_PATH", "tcp://*:6000");
    const auto cache_size = get_config<size_t>("ASGARD_CACHE_SIZE", 100000);
    const auto nb_threads = get_config<size_t>("ASGARD_NB_THREADS", 3);
    const auto valhalla_conf = get_config<std::string>("ASGARD_VALHALLA_CONF", "/data/valhalla/valhalla.conf");
    const auto metrics_binding = get_config<std::string>("ASGARD_METRICS_BINDING", "127.0.0.1:8080");

    boost::thread_group threads;
    zmq::context_t context(1);
    LoadBalancer lb(context);
    lb.bind(socket_path, "inproc://workers");
    const asgard::Metrics metrics(metrics_binding);
    const asgard::Projector projector(cache_size);

    ptree::ptree ptree;
    ptree::read_json(valhalla_conf, ptree);

    for (size_t i = 0; i < nb_threads; ++i) {
        threads.create_thread(std::bind(&worker, asgard::Context(context, ptree, cache_size, metrics, projector)));
    }

    // Connect worker threads to client threads via a queue
    while (true) {
        try {
            lb.run();
        } catch (const zmq::error_t&) {} //lors d'un SIGHUP on restore la queue
    }

    return 0;
}
