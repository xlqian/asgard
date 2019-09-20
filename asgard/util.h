#pragma once

#include "asgard/response.pb.h"

#include <valhalla/proto/options.pb.h>
#include <valhalla/proto/trip.pb.h>

namespace valhalla {
namespace sif {
enum class TravelMode : uint8_t;
}
} // namespace valhalla

namespace asgard {

namespace util {

pbnavitia::StreetNetworkMode convert_valhalla_to_navitia_mode(const valhalla::sif::TravelMode& mode);

valhalla::sif::TravelMode convert_navitia_to_valhalla_mode(const std::string& mode);

size_t navitia_to_valhalla_mode_index(const std::string& mode);

valhalla::Costing convert_navitia_to_valhalla_costing(const std::string& costing);

pbnavitia::CyclePathType convert_valhalla_to_navitia_cycle_lane(const valhalla::TripLeg_CycleLane& cycle_lane);
} // namespace util

} // namespace asgard
