#include "util.h"

#include <valhalla/sif/costconstants.h>

using namespace valhalla;

namespace asgard {

namespace util {

pbnavitia::StreetNetworkMode convert_valhalla_to_navitia_mode(const sif::TravelMode& mode) {
    switch (mode) {
    case sif::TravelMode::kDrive:
        return pbnavitia::Car;

    case sif::TravelMode::kPedestrian:
        return pbnavitia::Walking;

    case sif::TravelMode::kBicycle:
        return pbnavitia::Bike;

    default:
        throw std::invalid_argument("Bad convert_valhalla_to_navitia_mode parameter");
    }
}

const std::map<std::string, sif::TravelMode> navitia_to_valhalla_mode_map = {
    {"walking", sif::TravelMode::kPedestrian},
    {"bike", sif::TravelMode::kBicycle},
    {"car", sif::TravelMode::kDrive},
    {"taxi", sif::TravelMode::kDrive},
};

sif::TravelMode convert_navitia_to_valhalla_mode(const std::string& mode) {
    return navitia_to_valhalla_mode_map.at(mode);
}

size_t navitia_to_valhalla_mode_index(const std::string& mode) {
    return static_cast<size_t>(navitia_to_valhalla_mode_map.at(mode));
}

const std::map<std::string, odin::Costing> navitia_to_valhalla_costing_map = {
    {"walking", odin::Costing::pedestrian},
    {"bike", odin::Costing::bicycle},
    {"car", odin::Costing::auto_},
    {"taxi", odin::Costing::taxi},
};

odin::Costing convert_navitia_to_valhalla_costing(const std::string& costing) {
    return navitia_to_valhalla_costing_map.at(costing);
}

} // namespace util

} // namespace asgard
