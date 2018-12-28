#pragma once

#include "asgard/request.pb.h"
#include "asgard/response.pb.h"

#include <valhalla/thor/pathinfo.h>
#include <valhalla/proto/trippath.pb.h>

namespace asgard {

class DirectPathResponseBuilder {
  public:
    DirectPathResponseBuilder(){}

    pbnavitia::Response build_journey_response(const pbnavitia::Request &request,
                                               const std::vector<valhalla::thor::PathInfo> &path_info_list,
                                               const valhalla::odin::TripPath& trip_path);

    private:
    void compute_metadata(pbnavitia::Journey& pb_journey);

};

} // namespace asgard