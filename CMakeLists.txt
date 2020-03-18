# Light

project(Light
  VERSION 0.0.0.1
  DESCRIPTION "A lean, portable C89 graphics library"
  HOMEPAGE_URL "https://github.com/phoenix-engine/light"
  LANGUAGES C
)

cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

# # If Light is being built as a subtree in a CMake project, don't use
# # CMake packaging tools.
# set(LIGHT_IS_SUBTREE
#   CACHE
#   BOOL
#   NO
# )

set(LIGHT_DEV
  CACHE
  BOOL
  NO
)

set(LIGHT_BUILD_TESTS
  CACHE
  BOOL
  ${LIGHT_DEV}
)

set(LIGHT_BUILD_SAMPLES
  CACHE
  BOOL
  ${LIGHT_DEV}
)

set(LIGHT_INSTALL_DIR
  CACHE
  PATH
  ${CMAKE_INSTALL_PREFIX}
)

if(LIGHT_DEV)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif(LIGHT_DEV)

if(LIGHT_BUILD_TESTS)
  add_subdirectory(tests)
endif(LIGHT_BUILD_TESTS)

if(LIGHT_BUILD_SAMPLES)
  add_subdirectory(samples)
endif(LIGHT_BUILD_SAMPLES)

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

install(
  FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/LightConfig.cmake
    ${CMAKE_BINARY_DIR}/LightConfigVersion.cmake
  DESTINATION ${PKG_PREFIX}
)
