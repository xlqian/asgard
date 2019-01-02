#pragma once

#include "asgard/request.pb.h"
#include "asgard/response.pb.h"

#include <valhalla/thor/pathinfo.h>

namespace asgard
{

namespace direct_path_response_builder
{

pbnavitia::Response build_journey_response(const pbnavitia::Request &request,
                                           const std::vector<valhalla::thor::PathInfo> &path_info_list,
                                           float total_length);

void compute_metadata(pbnavitia::Journey &pb_journey);

} // namespace direct_path_response_builder
} // namespace asgard