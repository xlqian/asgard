
#include "asgard/mode_costing.h"
#include "asgard/util.h"

using namespace valhalla;

namespace asgard {

static odin::DirectionsOptions make_default_directions_options() {
    odin::DirectionsOptions default_directions_options;
    for (int i = 0; i < 12; ++i) {
        default_directions_options.add_costing_options();
    }
    return default_directions_options;
}
static const odin::DirectionsOptions default_directions_options = make_default_directions_options();

static odin::DirectionsOptions
make_costing_option(const std::string& mode, float speed) {
    odin::DirectionsOptions options = default_directions_options;
    speed *= 3.6;

    rapidjson::Document doc;
    if (mode == "car") {
        sif::ParseAutoCostOptions(doc, "", options.mutable_costing_options(odin::Costing::auto_));
    } else if (mode == "bike") {
        sif::ParseBicycleCostOptions(doc, "", options.mutable_costing_options(odin::Costing::bicycle));
        options.mutable_costing_options(odin::Costing::bicycle)->set_cycling_speed(speed);
    } else {
        sif::ParsePedestrianCostOptions(doc, "", options.mutable_costing_options(odin::Costing::pedestrian));
        options.mutable_costing_options(odin::Costing::pedestrian)->set_walking_speed(speed);
    }

    return options;
}

ModeCosting::ModeCosting() : factory(), costing() {
    factory.Register(odin::Costing::auto_, sif::CreateAutoCost);
    factory.Register(odin::Costing::pedestrian, sif::CreatePedestrianCost);
    factory.Register(odin::Costing::bicycle, sif::CreateBicycleCost);

    costing[util::navitia_to_valhalla_mode_index("car")] = factory.Create(odin::Costing::auto_, default_directions_options);
    costing[util::navitia_to_valhalla_mode_index("walking")] = factory.Create(odin::Costing::pedestrian, default_directions_options);
    costing[util::navitia_to_valhalla_mode_index("bike")] = factory.Create(odin::Costing::bicycle, default_directions_options);
}

void ModeCosting::update_costing_for_mode(const std::string& mode, float speed) {
    costing[util::navitia_to_valhalla_mode_index(mode)] = factory.Create(
        util::convert_navitia_to_valhalla_costing(mode), make_costing_option(mode, speed));
}

const valhalla::sif::cost_ptr_t ModeCosting::get_costing_for_mode(const std::string& mode) const {
    return costing[util::navitia_to_valhalla_mode_index(mode)];
}

} // namespace asgard
