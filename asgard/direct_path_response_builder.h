#pragma once

#include "asgard/response.pb.h"

namespace pbnavitia {
class Request;
} // namespace pbnavitia

namespace valhalla {

namespace thor {
class PathInfo;
}

namespace midgard {
class PointLL;
}

namespace odin {
class TripPath;
class TripPath_Edge;
class TripPath_Node;
} // namespace odin

} // namespace valhalla

namespace asgard {

namespace direct_path_response_builder {

pbnavitia::Response build_journey_response(const pbnavitia::Request& request,
                                           const std::vector<valhalla::thor::PathInfo>& path_info_list,
                                           const valhalla::odin::TripPath& trip_path);

void set_extremity_pt_object(const valhalla::midgard::PointLL& geo_point, pbnavitia::PtObject* o);
void compute_metadata(pbnavitia::Journey& pb_journey);
void compute_geojson(const std::vector<valhalla::midgard::PointLL>& list_geo_points, pbnavitia::Section& s);
void compute_path_items(const valhalla::odin::TripPath& trip_path, pbnavitia::StreetNetwork* s);

void set_path_item_name(const valhalla::odin::TripPath_Edge& edge, pbnavitia::PathItem& path_item);
void set_path_item_length(const valhalla::odin::TripPath_Edge& edge, pbnavitia::PathItem& path_item);
void set_path_item_type(const valhalla::odin::TripPath_Edge& edge, pbnavitia::PathItem& path_item);
uint32_t set_path_item_duration(const valhalla::odin::TripPath_Node& node, uint32_t previous_node_elapsed_time, pbnavitia::PathItem& path_item);

} // namespace direct_path_response_builder
} // namespace asgard
