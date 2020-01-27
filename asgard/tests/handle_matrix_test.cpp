#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE handle_matrix_test

#include "tile_maker.h"

#include "utils/zmq.h"
#include "asgard/conf.h"
#include "asgard/context.h"
#include "asgard/handler.h"
#include "asgard/metrics.h"
#include "asgard/projector.h"
#include "asgard/request.pb.h"

#include <valhalla/midgard/pointll.h>

#include <boost/test/unit_test.hpp>

using namespace valhalla;

namespace asgard {

namespace config {

// Need the define this otherwise it won't compile
const char* asgard_build_type = "whatever";
const char* project_version = "whatever2";

} // namespace config

const std::string make_string_from_point(const midgard::PointLL& p) {
    return std::string("coord:") +
           std::to_string(p.first) +
           ":" +
           std::to_string(p.second);
}

void add_origin_or_dest_to_request(pbnavitia::LocationContext* location, const std::string& coord) {
    location->set_place(coord);
    location->set_access_duration(0);
}

BOOST_AUTO_TEST_CASE(handle_matrix_test) {
    tile_maker::TileMaker maker;
    maker.make_tile();

    zmq::context_t context(1);
    const Metrics metrics{boost::none};
    const Projector projector{10, 0, 0};

    boost::property_tree::ptree conf;
    conf.put("tile_dir", maker.get_tile_dir());
    valhalla::baldr::GraphReader graph(conf);
    Context c{context, graph, metrics, projector};

    Handler h{c};

    pbnavitia::Request request;
    request.set_requested_api(pbnavitia::street_network_routing_matrix);
    auto* sn_request = request.mutable_sn_routing_matrix();

    add_origin_or_dest_to_request(sn_request->add_origins(),
                                  make_string_from_point(maker.get_all_points().front()));

    for (auto const& p : maker.get_all_points()) {
        add_origin_or_dest_to_request(sn_request->add_destinations(), make_string_from_point(p));
    }

    sn_request->set_mode("walking");
    sn_request->set_max_duration(100000);
    sn_request->set_speed(2);

    const auto response = h.handle(request);

    std::vector<unsigned int> expected_times = {0, 111, 444, 667, 359, 568};
    pbnavitia::Response expected_response;
    auto* row = expected_response.mutable_sn_routing_matrix()->add_rows();
    for(auto const& time : expected_times) {
        auto* k = row->add_routing_response();
        k->set_routing_status(pbnavitia::RoutingStatus::reached);
        k->set_duration(time);
    }

    BOOST_CHECK_EQUAL(expected_response.DebugString(), response.DebugString());
}

} // namespace asgard
