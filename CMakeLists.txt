# Light

########################################################################
# CMake policies
########################################################################

cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

# OLD: project() command manages VERSION variables.
if(POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif(POLICY CMP0048)

# OLD: target_sources() command converts relative paths to absolute.
if(POLICY CMP0076)
  cmake_policy(SET CMP0076 NEW)
endif(POLICY CMP0076)

# OLD: Targets need to be created in the same directory to be used.
if(POLICY CMP0079)
  cmake_policy(SET CMP0079 NEW)
endif(POLICY CMP0079)

########################################################################
# CMake Project
########################################################################

project(Light
  VERSION 0.0.0.1
  DESCRIPTION "A lean, portable C89 graphics library"
  HOMEPAGE_URL "https://github.com/phoenix-engine/light"
  LANGUAGES C
)

########################################################################
# CMake setup
########################################################################

if(WIN32)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
  set(CXX_EXTENSIONS OFF)
endif(WIN32)

set(LIGHT_DEV
  CACHE
  BOOL
  OFF
  "Enable dev mode for Light."
)

set(LIGHT_BUILD_TESTS
  CACHE
  BOOL
  ${LIGHT_DEV}
  "Enable tests for Light."
)

set(LIGHT_BUILD_SAMPLES
  CACHE
  BOOL
  ${LIGHT_DEV}
  "Enable samples for Light."
)

set(LIGHT_INSTALL_DIR
  CACHE
  PATH
  ${CMAKE_INSTALL_PREFIX}
  "The path where Light will be installed."
)

# set(SDL2_INSTALL_PREFIX
#   CACHE
#   PATH
#   "The path where SDL2 is installed."
# )

set(VULKAN_INSTALL_PREFIX
  CACHE
  PATH
  "$ENV{VULKAN_SDK}"
  "(Optional) The path where SDL2 is installed."
)

if($CACHE{LIGHT_DEV})
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif($CACHE{LIGHT_DEV})

if($CACHE{LIGHT_BUILD_TESTS})
  add_subdirectory(tests)
endif($CACHE{LIGHT_BUILD_TESTS})

if($CACHE{LIGHT_BUILD_SAMPLES})
  add_subdirectory(samples)
endif($CACHE{LIGHT_BUILD_SAMPLES})

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")
include(MacroEnsureOutOfSourceBuild)
macro_ensure_out_of_source_build("\
${PROJECT_NAME} requires an out of source build. Please create a \
separate build directory and run \
'cmake /path/to/${PROJECT_NAME} [options]' there."
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file("${CMAKE_BINARY_DIR}/LightConfigVersion.cmake"
  VERSION ${LIGHT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

set(LIGHT_DIR ${CMAKE_CURRENT_LIST_DIR})
set(INCLUDE_INSTALL_DIR include/)
set(LIB_INSTALL_DIR lib/)

configure_package_config_file(
  # The cmake.in file is the template file which will be elaborated.
  LightConfig.cmake.in

  # The .cmake file is the product of the template elaboration.  It will
  # be consumed by a downstream build which imports this project.
  LightConfig.cmake

  # INSTALL_DESTINATION is the path where the generated Config.cmake
  # file will be installed.
  INSTALL_DESTINATION ${CMAKE_CURRENT_LIST_DIR}/cmake

  # PATH_VARS specifies the install paths of the products of this Config
  # package.
  # PATH_VARS artifacts/Release
  PATH_VARS INCLUDE_INSTALL_DIR LIB_INSTALL_DIR

  # [NO_SET_AND_CHECK_MACRO]
  # NO_CHECK_REQUIRED_COMPONENTS_MACRO

  # INSTALL_PREFIX specifies the location the package will be installed.
  # In this case, the package is intended to be used from within the
  # build tree, rather than from a system lib directory.
  #
  # A future binary-only build could set this to a system lib install
  # directory.
  INSTALL_PREFIX ${CMAKE_CURRENT_LIST_DIR}
)

write_basic_package_version_file(
  LightConfigVersion.cmake
  VERSION 0.0.1
  COMPATIBILITY SameMinorVersion
)

########################################################################
# Dependencies
########################################################################

# We need SDL2 and Vulkan.  If the user already added them as targets,
# great.  Otherwise, try to use find_package.
if(NOT(TARGET SDL2))
  find_package(SDL2 CONFIG REQUIRED)
endif(NOT(TARGET SDL2))

if(NOT(TARGET Vulkan::Headers))
  find_package(Vulkan-Headers CONFIG REQUIRED)
endif(NOT(TARGET Vulkan::Headers))

########################################################################
# Targets
########################################################################

# light.h
add_library(Light SHARED)
target_sources(Light PUBLIC light.c)
target_sources(Light INTERFACE light.h)
target_include_directories(Light PUBLIC "${LIGHT_DIR}")

target_link_libraries(Light SDL2)
target_link_libraries(Light Vulkan::Headers)

set_property(TARGET Light PROPERTY C_STANDARD 90)
set_property(TARGET Light PROPERTY C_STANDARD_REQUIRED ON)

########################################################################
# Installation
########################################################################

# Export files (shamelessly cribbed from SDL2)
if (WINDOWS)
  set(PKG_PREFIX "cmake")
else ()
  set(PKG_PREFIX "lib/cmake/light")
endif ()

install(
  TARGETS
    Light
  EXPORT
    Light
  DESTINATION ${PKG_PREFIX}
)

# install(EXPORT Light DESTINATION ${PKG_PREFIX})

# install(
#   FILES
#     ${CMAKE_CURRENT_SOURCE_DIR}/LightConfig.cmake
#     ${CMAKE_BINARY_DIR}/LightConfigVersion.cmake
#   DESTINATION ${PKG_PREFIX}
# )
