#pragma once

#include <valhalla/loki/search.h>
#include <valhalla/baldr/graphreader.h>
#include <valhalla/baldr/location.h>
//#include <valhalla/sif/costfactory.h>
//#include <valhalla/sif/costconstants.h>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>

namespace asgard{


class wrong_coordinate: public std::runtime_error{using runtime_error::runtime_error;};

namespace {
    valhalla::baldr::Location build_location(const std::string& place){
        size_t pos2 = place.find(":", 6);
        try{
            if(pos2 != std::string::npos) {
                double lon = boost::lexical_cast<double>(place.substr(6, pos2 - 6));
                double lat =  boost::lexical_cast<double>(place.substr(pos2+1));
                auto l = valhalla::baldr::Location({lon, lat}, valhalla::baldr::Location::StopType::BREAK);
                l.name_ = place;
                return l;
            }
        }catch(const boost::bad_lexical_cast&){
            throw wrong_coordinate("conversion failed");
        }
        throw wrong_coordinate("not a coordinate");
    }
}

class Projector {
private:
    typedef std::string key_type;
    typedef valhalla::baldr::PathLocation mapped_type;
    typedef std::pair<const key_type, mapped_type> value_type;
    typedef boost::multi_index_container<
        value_type,
        boost::multi_index::indexed_by<
            boost::multi_index::sequenced<>,
            boost::multi_index::hashed_unique<
                boost::multi_index::member<value_type, const key_type, &value_type::first> > >
        > Cache;

    // maximal cached values
    size_t cache_size;

    // the cache, mutable because side effect are not visible from the
    // exterior because of the purity of f
    mutable Cache cache;
    mutable size_t nb_cache_miss = 0;
    mutable size_t nb_calls = 0;

public:

    Projector(size_t cache_size = 1000){
        if (cache_size < 1) {
            throw std::invalid_argument("max (size of cache) must be strictly positive");
        }
    }

    template <typename T>
    std::unordered_map<std::string,
                      valhalla::baldr::PathLocation> operator()(const T places_begin, const T places_end,
                                                                valhalla::baldr::GraphReader& graph,
                                                                valhalla::sif::cost_ptr_t costing) const{
    std::unordered_map<std::string, valhalla::baldr::PathLocation> results;
    std::vector<valhalla::baldr::Location> missed;
    auto& list = cache.template get<0>();
    auto& map = cache.template get<1>();
    for(auto it = places_begin; it != places_end; ++it){
        ++nb_calls;
        const auto search = map.find(*it);
        if (search != map.end()) {
            // put the cached value at the begining of the cache
            list.relocate(list.begin(), cache.template project<0>(search));
            results.emplace(*it, search->second);
        } else {
            ++nb_cache_miss;
            missed.push_back(build_location(*it));
        }
    }
    if(!missed.empty()){
        auto path_locations = valhalla::loki::Search(missed, graph, costing->GetEdgeFilter(), costing->GetNodeFilter());
        for(const auto& l: path_locations){
            list.push_front(std::make_pair(l.first.name_, l.second));
            results.emplace(l.first.name_, l.second);
        }
        while (list.size() > cache_size) { list.pop_back(); }
    }
    return results;
}


    size_t get_nb_cache_miss() const { return nb_cache_miss; }
    size_t get_nb_calls() const { return nb_calls; }
};
}
