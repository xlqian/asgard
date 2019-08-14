# - Check for the presence of VALHALLA
#
# The following variables are set when VALHALLA is found:
#  HAVE_VALHALLA       = Set to true, if all components of VALHALLA
#                          have been found.
#  VALHALLA_INCLUDES   = Include path for the header files of VALHALLA
#  VALHALLA_LIBRARIES  = Link these to use VALHALLA

# This module reads hints about search locations from variables::
#
#   VALHALLA_INCLUDEDIR       - Preferred include directory e.g. <prefix>/include
#   VALHALLA_LIBRARYDIR       - Preferred library directory e.g. <prefix>/lib

## -----------------------------------------------------------------------------
## Check for the header files

include(FindPkgConfig)
pkg_check_modules(libvalhalla QUIET libvalhalla>=3.0.0)
find_path (VALHALLA_INCLUDES valhalla/valhalla.h
    PATHS /usr/local/include /usr/include /sw/include ${VALHALLA_INCLUDEDIR} ${libvalhalla_INCLUDE_DIRS}
)

## -----------------------------------------------------------------------------
## Check for the library

find_library (VALHALLA_LIBRARIES libvalhalla.a
  PATHS /usr/local/lib /usr/lib /lib /sw/lib ${VALHALLA_LIBRARYDIR} ${libvalhalla_LIBRARY_DIRS}
)

## -----------------------------------------------------------------------------
## Actions taken when all components have been found

if (VALHALLA_INCLUDES AND VALHALLA_LIBRARIES)
  set (HAVE_VALHALLA TRUE)
else (VALHALLA_INCLUDES AND VALHALLA_LIBRARIES)
  if (NOT VALHALLA_FIND_QUIETLY)
    if (NOT VALHALLA_INCLUDES)
      message (STATUS "Unable to find VALHALLA header files!")
    endif (NOT VALHALLA_INCLUDES)
    if (NOT VALHALLA_LIBRARIES)
      message (STATUS "Unable to find VALHALLA library files!")
    endif (NOT VALHALLA_LIBRARIES)
  endif (NOT VALHALLA_FIND_QUIETLY)
endif (VALHALLA_INCLUDES AND VALHALLA_LIBRARIES)

if (HAVE_VALHALLA)
  if (NOT VALHALLA_FIND_QUIETLY)
    message (STATUS "Found components for valhalla")
    message (STATUS "VALHALLA_INCLUDES = ${VALHALLA_INCLUDES}")
    message (STATUS "VALHALLA_LIBRARIES     = ${VALHALLA_LIBRARIES}")
  endif (NOT VALHALLA_FIND_QUIETLY)
else (HAVE_VALHALLA)
  if (VALHALLA_FIND_REQUIRED)
    message (FATAL_ERROR "Could not find VALHALLA!")
  endif (VALHALLA_FIND_REQUIRED)
endif (HAVE_VALHALLA)

mark_as_advanced (
  HAVE_VALHALLA
  VALHALLA_LIBRARIES
  VALHALLA_INCLUDES
)
