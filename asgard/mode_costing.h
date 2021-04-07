#pragma once

#include <valhalla/sif/costfactory.h>
#include <valhalla/sif/dynamiccost.h>

namespace asgard {

static const size_t mode_costing_size = static_cast<size_t>(valhalla::sif::TravelMode::kMaxTravelMode);

using Costing = valhalla::sif::mode_costing_t;

struct ModeCostingArgs {
    std::string mode = "";
    std::vector<float> speeds;
    float bss_rent_cost = 120;
    float bss_rent_penalty = 0;
    float bss_return_cost = 120;
    float bss_return_penalty = 0;
    ModeCostingArgs() {
        mode = "";
        speeds = std::vector<float>(valhalla::Costing_MAX, 0);
        bss_rent_cost = 120;
        bss_rent_penalty = 0;
        bss_return_cost = 120;
        bss_return_penalty = 0;
    }
};

class ModeCosting {
public:
    ModeCosting();
    ModeCosting(const ModeCosting&) = delete;

    void update_costing(const ModeCostingArgs& args);

    const valhalla::sif::cost_ptr_t get_costing_for_mode(const std::string& mode) const;

    const Costing& get_costing() const { return costing; }

private:
    valhalla::sif::CostFactory factory;
    Costing costing;
};

} // namespace asgard
