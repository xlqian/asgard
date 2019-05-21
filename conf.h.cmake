#pragma once

#cmakedefine HAVE_ICONV_H 1
#cmakedefine HAVE_LOGGINGMACROS_H 1

namespace asgard { namespace config {

extern const char* asgard_build_type;
extern const char* project_version;

}}// namespace asgard::config
