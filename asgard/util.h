#pragma once

#include "asgard/response.pb.h"

#include <valhalla/sif/costconstants.h>

namespace asgard {

namespace util {

// Should use the map in handler.cpp ?
pbnavitia::StreetNetworkMode convert_valhalla_to_navitia_mode(const valhalla::sif::TravelMode& mode);

}

}