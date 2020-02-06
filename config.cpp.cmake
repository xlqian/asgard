#include "conf.h"

namespace asgard { 
namespace config {

const char* asgard_build_type = "${CMAKE_BUILD_TYPE}";
const char* project_version = "${GIT_REVISION}";

} // namespace config
} // namespace asgard
