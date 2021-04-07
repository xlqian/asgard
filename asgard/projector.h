#pragma once

#include "utils/coord_parser.h"

#include <valhalla/loki/search.h>
#include <valhalla/midgard/pointll.h>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>

#include <mutex>

namespace asgard {

class Projector {
private:
    friend class UnitTestProjector;

    typedef std::pair<valhalla::midgard::PointLL, std::string> key_type;
    typedef valhalla::baldr::PathLocation mapped_type;
    typedef std::pair<const key_type, mapped_type> value_type;
    typedef boost::multi_index_container<
        value_type,
        boost::multi_index::indexed_by<
            boost::multi_index::sequenced<>,
            boost::multi_index::hashed_unique<
                boost::multi_index::member<value_type, const key_type, &value_type::first>>>>
        Cache;

    // maximal cached values
    size_t cache_size;

    unsigned int min_outbound_reach;
    unsigned int min_inbound_reach;

    unsigned int radius;

    // the cache, mutable because side effect are not visible from the
    // exterior because of the purity of f
    mutable Cache cache;
    mutable size_t nb_cache_miss = 0;
    mutable size_t nb_cache_calls = 0;
    mutable std::mutex mutex;

    valhalla::baldr::Location build_location(const valhalla::midgard::PointLL& place,
                                             unsigned int min_outbound_reach,
                                             unsigned int min_inbound_reach,
                                             unsigned int radius) const {
        auto l = valhalla::baldr::Location(place,
                                           valhalla::baldr::Location::StopType::BREAK,
                                           min_outbound_reach,
                                           min_inbound_reach,
                                           radius);
        return l;
    }

public:
    explicit Projector(size_t cache_size = 1000,
                       unsigned int min_outbound_reach = 0,
                       unsigned int min_inbound_reach = 0,
                       unsigned int radius = 0) : cache_size(cache_size),
                                                  min_outbound_reach(min_outbound_reach),
                                                  min_inbound_reach(min_inbound_reach),
                                                  radius(radius) {}

    template<typename T>
    std::unordered_map<valhalla::midgard::PointLL, valhalla::baldr::PathLocation>
    operator()(const T places_begin,
               const T places_end,
               valhalla::baldr::GraphReader& graph,
               const std::string& mode,
               const valhalla::sif::cost_ptr_t& costing,
               const bool use_cache = true) const {
        if (use_cache) {
            return project_with_cache(places_begin, places_end, graph, mode, costing);
        }
        return project_without_cache(places_begin, places_end, graph, mode, costing);
    }

    size_t get_nb_cache_miss() const { return nb_cache_miss; }
    size_t get_nb_cache_calls() const { return nb_cache_calls; }
    size_t get_current_cache_size() const { return cache.template get<0>().size(); }

private:
    template<typename T>
    std::unordered_map<valhalla::midgard::PointLL, valhalla::baldr::PathLocation>
    project_with_cache(const T places_begin,
                       const T places_end,
                       valhalla::baldr::GraphReader& graph,
                       const std::string& mode,
                       const valhalla::sif::cost_ptr_t& costing) const {
        std::unordered_map<valhalla::midgard::PointLL, valhalla::baldr::PathLocation> results;
        std::vector<valhalla::baldr::Location> missed;
        auto& list = cache.template get<0>();
        const auto& map = cache.template get<1>();
        auto projector_mode = mode;
        if (projector_mode == "bss") {
            projector_mode = "walking";
        }
        {
            std::lock_guard<std::mutex> lock(mutex);
            for (auto it = places_begin; it != places_end; ++it) {
                ++nb_cache_calls;
                const auto search = map.find(std::make_pair(*it, projector_mode));
                if (search != map.end()) {
                    // put the cached value at the begining of the cache
                    list.relocate(list.begin(), cache.template project<0>(search));
                    results.emplace(*it, search->second);
                } else {
                    ++nb_cache_miss;
                    missed.push_back(build_location(*it, min_outbound_reach, min_inbound_reach, radius));
                }
            }
        }
        if (!missed.empty()) {
            const auto path_locations = valhalla::loki::Search(missed,
                                                               graph,
                                                               costing);

            std::lock_guard<std::mutex> lock(mutex);
            for (const auto& l : path_locations) {
                list.push_front(std::make_pair(std::make_pair(l.first.latlng_, projector_mode), l.second));
                results.emplace(l.first.latlng_, l.second);
            }
            while (list.size() > cache_size) { list.pop_back(); }
        }
        return results;
    }

    template<typename T>
    std::unordered_map<valhalla::midgard::PointLL, valhalla::baldr::PathLocation>
    project_without_cache(const T places_begin,
                          const T places_end,
                          valhalla::baldr::GraphReader& graph,
                          const std::string& mode,
                          const valhalla::sif::cost_ptr_t& costing) const {
        auto projector_mode = mode;
        if (projector_mode == "bss") {
            projector_mode = "walking";
        }
        std::vector<valhalla::baldr::Location> locations;
        std::transform(places_begin, places_end, std::back_inserter(locations),
                       [this](const valhalla::midgard::PointLL& place) {
                           return build_location(place, min_outbound_reach, min_inbound_reach, radius);
                       });
        const auto path_locations = valhalla::loki::Search(locations,
                                                           graph,
                                                           costing);

        std::unordered_map<valhalla::midgard::PointLL, valhalla::baldr::PathLocation> results;
        for (const auto& l : path_locations) {
            results.emplace(l.first.latlng_, l.second);
        }
        return results;
    }
};

} // namespace asgard
