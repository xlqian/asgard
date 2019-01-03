#pragma once

#include "utils/coord_parser.h"

#include <valhalla/loki/search.h>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>

namespace asgard {

namespace {
valhalla::baldr::Location build_location(const std::string& place) {
    auto coord = navitia::parse_coordinate(place);
    auto l = valhalla::baldr::Location({coord.first, coord.second},
                                       valhalla::baldr::Location::StopType::BREAK);
    l.name_ = place;
    return l;
}
} // namespace

class Projector {
  private:
    typedef std::pair<std::string, std::string> key_type;
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

    // the cache, mutable because side effect are not visible from the
    // exterior because of the purity of f
    mutable Cache cache;
    mutable size_t nb_cache_miss = 0;
    mutable size_t nb_calls = 0;

  public:
    Projector(size_t cache_size = 1000) : cache_size(cache_size) {}

    template<typename T>
    std::unordered_map<std::string, valhalla::baldr::PathLocation>
    operator()(const T places_begin,
               const T places_end,
               valhalla::baldr::GraphReader& graph,
               const std::string& mode,
               valhalla::sif::cost_ptr_t costing) const {
        std::unordered_map<std::string, valhalla::baldr::PathLocation> results;
        std::vector<valhalla::baldr::Location> missed;
        auto& list = cache.template get<0>();
        auto& map = cache.template get<1>();
        for (auto it = places_begin; it != places_end; ++it) {
            ++nb_calls;
            const auto search = map.find(std::make_pair(*it, mode));
            if (search != map.end()) {
                // put the cached value at the begining of the cache
                list.relocate(list.begin(), cache.template project<0>(search));
                results.emplace(*it, search->second);
            } else {
                ++nb_cache_miss;
                missed.push_back(build_location(*it));
            }
        }
        if (!missed.empty()) {
            auto path_locations = valhalla::loki::Search(missed,
                                                         graph,
                                                         costing->GetEdgeFilter(),
                                                         costing->GetNodeFilter());
            for (const auto& l : path_locations) {
                list.push_front(std::make_pair(std::make_pair(l.first.name_, mode), l.second));
                results.emplace(l.first.name_, l.second);
            }
            while (list.size() > cache_size) { list.pop_back(); }
        }
        return results;
    }

    size_t get_nb_cache_miss() const { return nb_cache_miss; }
    size_t get_nb_calls() const { return nb_calls; }
};

} // namespace asgard
