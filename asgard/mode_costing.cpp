
#include "asgard/mode_costing.h"
#include "asgard/util.h"

#include <valhalla/proto/options.pb.h>
#include <valhalla/sif/costconstants.h>

using namespace valhalla;
using vc = valhalla::Costing;

namespace asgard {

namespace {

Options make_default_directions_options(const std::string& mode) {
    Options default_directions_options;

    valhalla::Costing costing = util::convert_navitia_to_valhalla_costing(mode);
    default_directions_options.set_costing(costing);

    rapidjson::Document doc;
    sif::ParseCostingOptions(doc, "/costing_options", default_directions_options);

    return default_directions_options;
}

// TODO: for BSS mode, multimodal speed must be correctly handled
Options make_costing_option(const std::string& mode, float speed) {
    Options options = make_default_directions_options(mode);

    speed *= 3.6;

    rapidjson::Document doc;
    if (mode == "car") {
        sif::ParseAutoCostOptions(doc, "", options.mutable_costing_options(vc::auto_));
        options.mutable_costing_options(vc::auto_)->set_top_speed(speed);
    } else if (mode == "taxi") {
        sif::ParseTaxiCostOptions(doc, "", options.mutable_costing_options(vc::taxi));
        options.mutable_costing_options(vc::auto_)->set_top_speed(speed);
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
}

void ModeCosting::update_costing_for_mode(const std::string& mode, float speed) {
    auto new_options = make_costing_option(mode, speed);
    auto travel_mode = util::convert_navitia_to_valhalla_mode(mode);

    costing = factory.CreateModeCosting(new_options, travel_mode);
}

const valhalla::sif::cost_ptr_t ModeCosting::get_costing_for_mode(const std::string& mode) const {
    return costing[util::navitia_to_valhalla_mode_index(mode)];
}

} // namespace asgard
