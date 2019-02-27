#pragma once

#include <memory>
#include <cstdio>

#include "asgard/response.pb.h"
#include <valhalla/proto/directions_options.pb.h>

namespace valhalla {
namespace sif {
enum class TravelMode : uint8_t;
}
} // namespace valhalla

namespace asgard {

namespace util {

pbnavitia::StreetNetworkMode convert_valhalla_to_navitia_mode(const valhalla::sif::TravelMode& mode);

valhalla::sif::TravelMode convert_navitia_to_valhalla_mode(const std::string& mode);

size_t navitia_to_valhalla_mode_index(const std::string& mode);

valhalla::odin::Costing convert_navitia_to_valhalla_costing(const std::string& costing);

template<typename ... Args>
std::string string_format( const std::string& format, Args ... args )
{
    size_t size = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
    std::unique_ptr<char[]> buf( new char[ size ] );
    std::snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}

} // namespace util

} // namespace asgard
