#include "util.h"

using namespace valhalla;

namespace asgard {

namespace util {

pbnavitia::StreetNetworkMode convert_valhalla_to_navitia_mode(const sif::TravelMode& mode)
{    
    switch (mode)
    {
        case sif::TravelMode::kDrive:
            return pbnavitia::Car;
            break;

        case sif::TravelMode::kPedestrian:
            return pbnavitia::Walking;
            break;

        case sif::TravelMode::kBicycle:
            return pbnavitia::Bike;
            break;
    
        default:
            throw std::invalid_argument("Bad convert_valhalla_to_navitia_mode parameter");
            break;
    }
}

}

}