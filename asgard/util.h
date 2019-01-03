#pragma once

#include "asgard/response.pb.h"

namespace valhalla {
    namespace sif {
        enum class TravelMode : uint8_t;
    }
}

namespace asgard {

namespace util {

// Should use the map in handler.cpp ?
pbnavitia::StreetNetworkMode convert_valhalla_to_navitia_mode(const valhalla::sif::TravelMode& mode);

}

}