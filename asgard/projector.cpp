#include "asgard/projector.h"

namespace asgard{



static valhalla::baldr::Location build_location(const std::string& place){
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

Projector::Projector(size_t cache_size): cache_size(cache_size) {
    if (cache_size < 1) {
        throw std::invalid_argument("max (size of cache) must be strictly positive");
    }
}

std::unordered_map<std::string, valhalla::baldr::PathLocation>
Projector::operator()(const std::vector<std::string>& places,
                                        valhalla::baldr::GraphReader& graph,
                                        valhalla::sif::cost_ptr_t costing) const {
    std::unordered_map<std::string, valhalla::baldr::PathLocation>  results;
    std::vector<valhalla::baldr::Location> missed;
    auto& list = cache.template get<0>();
    auto& map = cache.template get<1>();
    for(const auto& place: places){
        ++nb_calls;
        const auto search = map.find(place);
        if (search != map.end()) {
            // put the cached value at the begining of the cache
            list.relocate(list.begin(), cache.template project<0>(search));
            results.emplace(place, search->second);
        } else {
            ++nb_cache_miss;
            missed.push_back(build_location(place));
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

}
