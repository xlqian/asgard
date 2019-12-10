#pragma once

#include "asgard/response.pb.h"

namespace pbnavitia {
class Request;
} // namespace pbnavitia

namespace valhalla {

class TripLeg;
class TripLeg_Edge;
class TripLeg_Node;

namespace thor {
class PathInfo;
}

namespace midgard {
class PointLL;
}

} // namespace valhalla

namespace asgard {

namespace direct_path_response_builder {

pbnavitia::Response build_journey_response(const pbnavitia::Request& request,
                                           const std::vector<valhalla::thor::PathInfo>& pathedges,
                                           const valhalla::TripLeg& trip_leg);

void set_extremity_pt_object(const valhalla::midgard::PointLL& geo_point, pbnavitia::PtObject* o);
void compute_metadata(pbnavitia::Journey& pb_journey);
void compute_geojson(const std::vector<valhalla::midgard::PointLL>& list_geo_points, pbnavitia::Section& s);
void compute_path_items(const valhalla::TripLeg& trip_leg, pbnavitia::StreetNetwork* sn);

void set_path_item_name(const valhalla::TripLeg_Edge& edge, pbnavitia::PathItem& path_item);
void set_path_item_length(const valhalla::TripLeg_Edge& edge, pbnavitia::PathItem& path_item);
void set_path_item_type(const valhalla::TripLeg_Edge& edge, pbnavitia::PathItem& path_item);
uint32_t set_path_item_duration(const valhalla::TripLeg_Node& node, uint32_t previous_node_elapsed_time, pbnavitia::PathItem& path_item);

} // namespace direct_path_response_builder
} // namespace asgard
