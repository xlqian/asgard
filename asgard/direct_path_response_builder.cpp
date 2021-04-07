#include "asgard/direct_path_response_builder.h"
#include "asgard/request.pb.h"
#include "asgard/util.h"

#include <valhalla/midgard/encoded.h>
#include <valhalla/midgard/logging.h>
#include <valhalla/midgard/pointll.h>
#include <valhalla/thor/pathinfo.h>

#include <boost/range/algorithm/find_if.hpp>
#include <numeric>

using namespace valhalla;

namespace asgard {

namespace direct_path_response_builder {

constexpr static float KM_TO_M = 1000.f;

void make_section(pbnavitia::Journey& journey,
                  valhalla::Api& api,
                  const DirectionsLeg& directions_leg,
                  const std::vector<midgard::PointLL>& shape,
                  ConstManeuverItetator begin_maneuver,
                  ConstManeuverItetator end_maneuver,
                  const time_t begin_date_time,
                  const pbnavitia::StreetNetworkParams& request_params,
                  const pbnavitia::SectionType section_type,
                  const size_t nb_sections,
                  const bool enable_instructions) {
    using BssManeuverType = DirectionsLeg_Maneuver_BssManeuverType;

    auto rent_cost = request_params.bss_rent_cost();
    ;
    auto return_cost = request_params.bss_return_cost();

    auto* section = journey.add_sections();
    section->set_type(section_type);
    section->set_id("section_" + std::to_string(nb_sections));
    auto* sn = section->mutable_street_network();

    auto travel_mode = begin_maneuver->travel_mode();
    sn->set_mode(util::convert_valhalla_to_navitia_mode(travel_mode));

    float section_duration = 0;
    float section_length = 0;
    std::string bss_maneuver_instructions;

    switch (section_type) {
    case pbnavitia::BSS_RENT:
        section_duration = rent_cost;
        section_length = 0;
        bss_maneuver_instructions = "Rent a bike from bike share station.";
        break;
    case pbnavitia::BSS_PUT_BACK:
        section_duration = return_cost;
        section_length = 0;
        bss_maneuver_instructions = "Return the bike back to bike share station.";
        break;
    default:
        section_duration = std::accumulate(
            begin_maneuver,
            end_maneuver,
            0.f,
            [&](float sum, const auto& m) { return sum + m.time(); });

        section_length = std::accumulate(
            begin_maneuver,
            end_maneuver,
            0.f,
            [&](float sum, const auto& m) { return sum + m.length(); });

        if (begin_maneuver->bss_maneuver_type() ==
            BssManeuverType::DirectionsLeg_Maneuver_BssManeuverType_kRentBikeAtBikeShare) {
            section_duration -= rent_cost;
        } else if (begin_maneuver->bss_maneuver_type() ==
                   BssManeuverType::DirectionsLeg_Maneuver_BssManeuverType_kReturnBikeAtBikeShare) {
            section_duration -= return_cost;
        }

        break;
    }

    section->set_duration(section_duration);
    sn->set_duration(section_duration);

    section->set_length(section_length);
    sn->set_length(section_length);

    section->set_begin_date_time(begin_date_time);
    section->set_end_date_time(begin_date_time + section_duration);

    auto shape_begin_idx = begin_maneuver->begin_shape_index();
    set_extremity_pt_object(*(shape.begin() + shape_begin_idx), section->mutable_origin());

    size_t shape_end_idx;
    if (end_maneuver == directions_leg.maneuver().end()) {
        shape_end_idx = shape.size();
        set_extremity_pt_object(shape.back(), section->mutable_destination());
    } else {
        shape_end_idx = end_maneuver->begin_shape_index() + 1;
        set_extremity_pt_object(*(shape.begin() + shape_end_idx), section->mutable_destination());
    }

    compute_geojson({shape.begin() + shape_begin_idx,
                     shape.begin() + shape_end_idx},
                    *section);

    if (section_type == pbnavitia::BSS_RENT || section_type == pbnavitia::BSS_PUT_BACK) {
        auto path_item = sn->add_path_items();
        path_item->set_duration(section_duration);
        path_item->set_instruction("Put the bike back to the bike share station");
    } else {
        compute_path_items(api, section->mutable_street_network(), enable_instructions, begin_maneuver, end_maneuver);
    }
}

pbnavitia::Response build_journey_response(const pbnavitia::Request& request,
                                           const std::vector<valhalla::thor::PathInfo>& pathedges,
                                           const TripLeg& trip_leg,
                                           valhalla::Api& api) {
    pbnavitia::Response response;

    if (pathedges.empty() ||
        api.mutable_trip()->routes_size() == 0 || !api.has_directions() ||
        api.mutable_directions()->mutable_routes(0)->legs_size() == 0) {
        response.set_response_type(pbnavitia::NO_SOLUTION);
        LOG_ERROR("No solution found !");
        return response;
    }

    LOG_INFO("Building solution...");
    // General
    response.set_response_type(pbnavitia::ITINERARY_FOUND);

    // Journey
    auto const& request_params = request.direct_path().streetnetwork_params();

    auto* journey = response.mutable_journeys()->Add();
    size_t nb_sections = 0;

    journey->set_duration(pathedges.back().elapsed_cost.secs);
    journey->set_nb_transfers(0);

    auto const departure_posix_time = time_t(request.direct_path().datetime());
    auto const arrival_posix_time = departure_posix_time + time_t(journey->duration());

    journey->set_requested_date_time(departure_posix_time);
    journey->set_departure_date_time(departure_posix_time);
    journey->set_arrival_date_time(arrival_posix_time);

    auto const shape = midgard::decode<std::vector<midgard::PointLL>>(trip_leg.shape());

    auto& directions_leg = *api.mutable_directions()->mutable_routes(0)->mutable_legs(0);

    auto previous_end_time = departure_posix_time;

    using BssManeuverType = DirectionsLeg_Maneuver_BssManeuverType;
    auto current_bss_maneuver_type = boost::optional<BssManeuverType>{};

    auto bss_rent_maneuver = boost::range::find_if(
        directions_leg.maneuver(),
        [](const auto& m) {
            return m.bss_maneuver_type() ==
                   BssManeuverType::DirectionsLeg_Maneuver_BssManeuverType_kRentBikeAtBikeShare;
        });

    auto bss_return_maneuver = boost::range::find_if(
        directions_leg.maneuver(),
        [](const auto& m) {
            return m.bss_maneuver_type() ==
                   BssManeuverType::DirectionsLeg_Maneuver_BssManeuverType_kReturnBikeAtBikeShare;
        });

    bool enable_instructions = request.direct_path().streetnetwork_params().enable_instructions();

    auto last_journey_section_duration = [journey]() {
        return std::next(journey->sections().end(), -1)->duration();
    };

    if (bss_rent_maneuver != directions_leg.maneuver().begin()) {
        // Case 1: the journey is a mono-modal solution, we need to create only one section.
        // Case 2: the journey is a multi-modal solution(BSS so far), the first section is walking
        make_section(*journey,
                     api,
                     directions_leg,
                     shape,
                     directions_leg.maneuver().begin(),
                     bss_rent_maneuver,
                     departure_posix_time,
                     request_params,
                     pbnavitia::STREET_NETWORK,
                     nb_sections++,
                     enable_instructions);
        previous_end_time += last_journey_section_duration();
    }

    if (bss_rent_maneuver != directions_leg.maneuver().end()) {
        // Case 1: the first section is a walking section, now we are building the bike rent section
        // Case 2: This is actually the first section, because the start point is projected on the bike
        //         share station

        // In both cases, we must build
        //   *  rent section
        //   *  the bike section
        //   *  return section

        // rent section
        make_section(*journey,
                     api,
                     directions_leg,
                     shape,
                     // Note that the begin and maneuver are the same
                     bss_rent_maneuver,
                     bss_rent_maneuver,
                     previous_end_time,
                     request_params,
                     pbnavitia::BSS_RENT,
                     nb_sections++,
                     enable_instructions);
        previous_end_time += last_journey_section_duration();

        // bike section
        make_section(*journey,
                     api,
                     directions_leg,
                     shape,
                     bss_rent_maneuver,
                     bss_return_maneuver,
                     previous_end_time,
                     request_params,
                     pbnavitia::STREET_NETWORK,
                     nb_sections++,
                     enable_instructions);
        previous_end_time += last_journey_section_duration();

        // return section
        make_section(*journey,
                     api,
                     directions_leg,
                     shape,
                     // Note that the begin and maneuver are the same
                     bss_return_maneuver,
                     bss_return_maneuver,
                     previous_end_time,
                     request_params,
                     pbnavitia::BSS_PUT_BACK,
                     nb_sections++,
                     enable_instructions);
        previous_end_time += last_journey_section_duration();
    }

    if (bss_return_maneuver != directions_leg.maneuver().end()) {
        // if return maneuver is not the section, in other words, the destination is not projected
        // on the bike share station, we have to add another section to get to the destination on foot

        // Walking section
        make_section(*journey,
                     api,
                     directions_leg,
                     shape,
                     bss_return_maneuver,
                     directions_leg.maneuver().end(),
                     previous_end_time,
                     request_params,
                     pbnavitia::STREET_NETWORK,
                     nb_sections++,
                     enable_instructions);
    }

    journey->set_nb_sections(nb_sections);

    compute_metadata(*journey);

    LOG_INFO("Solution built...");

    return response;
}

void set_extremity_pt_object(const valhalla::midgard::PointLL& geo_point, pbnavitia::PtObject* o) {
    auto uri = std::stringstream();
    uri << std::fixed << std::setprecision(5) << geo_point.lng() << ";" << geo_point.lat();
    o->set_uri(uri.str());
    o->set_name("");
    auto* coords = o->mutable_address()->mutable_coord();
    coords->set_lat(geo_point.lat());
    coords->set_lon(geo_point.lng());
}

void compute_geojson(const std::vector<midgard::PointLL>& list_geo_points, pbnavitia::Section& s) {
    for (const auto& point : list_geo_points) {
        auto* geo = s.mutable_street_network()->add_coordinates();
        geo->set_lat(point.lat());
        geo->set_lon(point.lng());
    }
}

void compute_metadata(pbnavitia::Journey& pb_journey) {
    uint32_t total_walking_duration = 0;
    uint32_t total_car_duration = 0;
    uint32_t total_bike_duration = 0;
    uint32_t total_ridesharing_duration = 0;
    uint32_t total_taxi_duration = 0;

    uint32_t total_walking_distance = 0;
    uint32_t total_car_distance = 0;
    uint32_t total_bike_distance = 0;
    uint32_t total_ridesharing_distance = 0;
    uint32_t total_taxi_distance = 0;

    for (const auto& section : pb_journey.sections()) {
        if (section.type() == pbnavitia::STREET_NETWORK || section.type() == pbnavitia::CROW_FLY) {
            switch (section.street_network().mode()) {
            case pbnavitia::StreetNetworkMode::Walking:
                total_walking_duration += section.duration();
                total_walking_distance += section.length();
                break;
            case pbnavitia::StreetNetworkMode::Car:
            case pbnavitia::StreetNetworkMode::CarNoPark:
                total_car_duration += section.duration();
                total_car_distance += section.length();
                break;
            case pbnavitia::StreetNetworkMode::Bike:
            case pbnavitia::StreetNetworkMode::Bss:
                total_bike_duration += section.duration();
                total_bike_distance += section.length();
                break;
            case pbnavitia::StreetNetworkMode::Ridesharing:
                total_ridesharing_duration += section.duration();
                total_ridesharing_distance += section.length();
                break;
            case pbnavitia::StreetNetworkMode::Taxi:
                total_taxi_duration += section.duration();
                total_taxi_distance += section.length();
                break;
            }
        } else if (section.type() == pbnavitia::TRANSFER && section.transfer_type() == pbnavitia::walking) {
            total_walking_duration += section.duration();
        }
    }

    const auto ts_departure = pb_journey.sections(0).begin_date_time();
    const auto ts_arrival = pb_journey.sections(pb_journey.sections_size() - 1).end_date_time();
    auto* durations = pb_journey.mutable_durations();
    durations->set_walking(total_walking_duration);
    durations->set_bike(total_bike_duration);
    durations->set_car(total_car_duration);
    durations->set_ridesharing(total_ridesharing_duration);
    durations->set_taxi(total_taxi_duration);
    durations->set_total(ts_arrival - ts_departure);

    auto* distances = pb_journey.mutable_distances();
    distances->set_walking(total_walking_distance);
    distances->set_bike(total_bike_distance);
    distances->set_car(total_car_distance);
    distances->set_ridesharing(total_ridesharing_distance);
    distances->set_taxi(total_taxi_distance);

    pb_journey.set_duration(ts_arrival - ts_departure);
}

void compute_path_items(valhalla::Api& api,
                        pbnavitia::StreetNetwork* sn,
                        const bool enable_instruction,
                        ConstManeuverItetator begin_maneuver,
                        ConstManeuverItetator end_maneuver) {

    auto& trip_route = *api.mutable_trip()->mutable_routes(0);
    auto& directions_leg = *api.mutable_directions()->mutable_routes(0)->mutable_legs(0);

    for (auto it = begin_maneuver; it < end_maneuver; ++it) {
        const auto& maneuver = *it;
        auto* path_item = sn->add_path_items();
        auto const& edge = trip_route.mutable_legs(0)->node(maneuver.begin_path_index()).edge();

        set_path_item_type(edge, *path_item);
        set_path_item_name(maneuver, *path_item);
        set_path_item_length(maneuver, *path_item);
        set_path_item_duration(maneuver, *path_item);
        set_path_item_direction(maneuver, *path_item);
        if (enable_instruction) {
            bool is_last_maneuver = std::distance(it, directions_leg.maneuver().end()) == 1;
            set_path_item_instruction(maneuver, *path_item, is_last_maneuver);
        }
    }
}

void set_path_item_instruction(const DirectionsLeg_Maneuver& maneuver, pbnavitia::PathItem& path_item, const bool is_last_maneuver) {
    if (maneuver.has_text_instruction() && !maneuver.text_instruction().empty()) {
        auto instruction = maneuver.text_instruction();
        if (!is_last_maneuver) {
            instruction += " Keep going for " + std::to_string((int)(maneuver.length() * KM_TO_M)) + " m.";
        }
        path_item.set_instruction(instruction);
    }
}

void set_path_item_name(const DirectionsLeg_Maneuver& maneuver, pbnavitia::PathItem& path_item) {
    if (!maneuver.street_name().empty() && maneuver.street_name().Get(0).has_value()) {
        path_item.set_name(maneuver.street_name().Get(0).value());
    }
}

void set_path_item_length(const DirectionsLeg_Maneuver& maneuver, pbnavitia::PathItem& path_item) {
    if (maneuver.has_length()) {
        path_item.set_length(maneuver.length() * KM_TO_M);
    }
}

// For now, we only handle cycle lanes
void set_path_item_type(const TripLeg_Edge& edge, pbnavitia::PathItem& path_item) {
    if (edge.has_cycle_lane()) {
        path_item.set_cycle_path_type(util::convert_valhalla_to_navitia_cycle_lane(edge.cycle_lane()));
    }
}

void set_path_item_duration(const DirectionsLeg_Maneuver& maneuver, pbnavitia::PathItem& path_item) {
    if (maneuver.has_time()) {
        path_item.set_duration(maneuver.time());
    }
}

void set_path_item_direction(const DirectionsLeg_Maneuver& maneuver, pbnavitia::PathItem& path_item) {
    if (maneuver.has_turn_degree()) {
        int turn_degree = maneuver.turn_degree() % 360;
        if (turn_degree > 180) turn_degree -= 360;
        path_item.set_direction(turn_degree);
    }
}

} // namespace direct_path_response_builder

} // namespace asgard
