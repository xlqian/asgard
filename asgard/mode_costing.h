#pragma once

#include <valhalla/sif/costfactory.h>
#include <valhalla/sif/dynamiccost.h>

namespace asgard {

static const size_t mode_costing_size = static_cast<size_t>(valhalla::sif::TravelMode::kMaxTravelMode);

using Costing = valhalla::sif::mode_costing_t;

class ModeCosting {
public:
    ModeCosting();
    ModeCosting(const ModeCosting&) = delete;

    void update_costing_for_mode(const std::string& mode, float speed);
    const valhalla::sif::cost_ptr_t get_costing_for_mode(const std::string& mode) const;

    const Costing& get_costing() const { return costing; }

private:
    valhalla::sif::CostFactory factory;
    Costing costing;
};

} // namespace asgard
