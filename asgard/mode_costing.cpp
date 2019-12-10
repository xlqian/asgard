
#include "asgard/mode_costing.h"
#include "asgard/util.h"

#include <valhalla/proto/options.pb.h>

using namespace valhalla;
using vc = valhalla::Costing;

namespace asgard {

namespace {

Options make_default_directions_options() {
    Options default_directions_options;
    // If a mode is added in valhalla and use in agard, we have to update this
    for (int i = 0; i < vc::taxi + 1; ++i) {
        default_directions_options.add_costing_options();
    }
    return default_directions_options;
}

Options make_costing_option(const std::string& mode, float speed) {
    Options options = make_default_directions_options();
    speed *= 3.6;

    rapidjson::Document doc;
    if (mode == "car") {
        sif::ParseAutoCostOptions(doc, "", options.mutable_costing_options(vc::auto_));
    } else if (mode == "taxi") {
        sif::ParseTaxiCostOptions(doc, "", options.mutable_costing_options(vc::taxi));
    } else if (mode == "bike") {
        sif::ParseBicycleCostOptions(doc, "", options.mutable_costing_options(vc::bicycle));
        options.mutable_costing_options(vc::bicycle)->set_cycling_speed(speed);
    } else {
        sif::ParsePedestrianCostOptions(doc, "", options.mutable_costing_options(vc::pedestrian));
        options.mutable_costing_options(vc::pedestrian)->set_walking_speed(speed);
    }

    return options;
}
} // namespace

ModeCosting::ModeCosting() {
    factory.Register(vc::auto_, sif::CreateAutoCost);
    factory.Register(vc::taxi, sif::CreateTaxiCost);
    factory.Register(vc::pedestrian, sif::CreatePedestrianCost);
    factory.Register(vc::bicycle, sif::CreateBicycleCost);

    const auto default_directions_options = make_default_directions_options();
    costing[util::navitia_to_valhalla_mode_index("car")] = factory.Create(vc::auto_, default_directions_options);
    costing[util::navitia_to_valhalla_mode_index("taxi")] = factory.Create(vc::taxi, default_directions_options);
    costing[util::navitia_to_valhalla_mode_index("walking")] = factory.Create(vc::pedestrian, default_directions_options);
    costing[util::navitia_to_valhalla_mode_index("bike")] = factory.Create(vc::bicycle, default_directions_options);
}

void ModeCosting::update_costing_for_mode(const std::string& mode, float speed) {
    costing[util::navitia_to_valhalla_mode_index(mode)] = factory.Create(
        util::convert_navitia_to_valhalla_costing(mode), make_costing_option(mode, speed));
}

const valhalla::sif::cost_ptr_t ModeCosting::get_costing_for_mode(const std::string& mode) const {
    return costing[util::navitia_to_valhalla_mode_index(mode)];
}

} // namespace asgard
