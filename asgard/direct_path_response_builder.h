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
}

} // namespace valhalla

namespace asgard {

namespace direct_path_response_builder {

pbnavitia::Response build_journey_response(const pbnavitia::Request& request,
                                           const std::vector<valhalla::thor::PathInfo>& path_info_list,
                                           const valhalla::odin::TripPath& trip_path);

void set_extremity_pt_object(const valhalla::midgard::PointLL& geo_point, pbnavitia::PtObject* o);
void compute_metadata(pbnavitia::Journey& pb_journey);
void compute_geojson(const std::vector<valhalla::midgard::PointLL>& list_geo_points, pbnavitia::Section& s);

} // namespace direct_path_response_builder
} // namespace asgard
