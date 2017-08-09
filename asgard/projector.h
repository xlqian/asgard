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

class Projector {
private:
    typedef typename std::string key_type;
    typedef typename valhalla::baldr::PathLocation mapped_type;
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

    Projector(size_t cache_size = 1000);

    std::unordered_map<std::string,
                      valhalla::baldr::PathLocation> operator()(const std::vector<std::string>& places,
                                                                valhalla::baldr::GraphReader& graph,
                                                                valhalla::sif::cost_ptr_t costing) const;


    size_t get_nb_cache_miss() const { return nb_cache_miss; }
    size_t get_nb_calls() const { return nb_calls; }
};
}
