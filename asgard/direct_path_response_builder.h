#pragma once

#include "asgard/response.pb.h"

namespace pbnavitia {
class Request;
class Response;
} // namespace pbnavitia

namespace valhalla {
namespace thor {
class PathInfo;
}
} // namespace valhalla::thor

namespace asgard {

namespace direct_path_response_builder {

pbnavitia::Response build_journey_response(const pbnavitia::Request& request,
                                           const std::vector<valhalla::thor::PathInfo>& path_info_list,
                                           float total_length);

void compute_metadata(pbnavitia::Journey& pb_journey);
}
} // namespace asgard::direct_path_response_builder