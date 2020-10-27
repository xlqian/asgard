#pragma once

#include "asgard/response.pb.h"
#include <valhalla/odin/directionsbuilder.h>

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
                                           const valhalla::TripLeg& trip_leg,
                                           valhalla::Api& api);

void set_extremity_pt_object(const valhalla::midgard::PointLL& geo_point, pbnavitia::PtObject* o);
void compute_metadata(pbnavitia::Journey& pb_journey);
void compute_geojson(const std::vector<valhalla::midgard::PointLL>& list_geo_points, pbnavitia::Section& s);
void compute_path_items(valhalla::Api& api, pbnavitia::StreetNetwork* sn);

void set_path_item_name(const valhalla::DirectionsLeg_Maneuver& maneuver, pbnavitia::PathItem& path_item);
void set_path_item_length(const valhalla::DirectionsLeg_Maneuver& maneuver, pbnavitia::PathItem& path_item);
void set_path_item_type(const valhalla::TripLeg_Edge& edge, pbnavitia::PathItem& path_item);
void set_path_item_duration(const valhalla::DirectionsLeg_Maneuver& maneuver, pbnavitia::PathItem& path_item);
void set_path_item_direction(const valhalla::DirectionsLeg_Maneuver& maneuver, pbnavitia::PathItem& path_item);
void set_path_item_instruction(const valhalla::DirectionsLeg_Maneuver& maneuver, pbnavitia::PathItem& path_item);

} // namespace direct_path_response_builder
} // namespace asgard
