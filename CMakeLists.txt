cmake_minimum_required(VERSION 3.20)

project(
  home_ems
  VERSION 0.1.0
  DESCRIPTION "Home battery dispatch between PV, load and grid"
  LANGUAGES CXX)

set(HOME_EMS_TOP_LEVEL OFF)
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(HOME_EMS_TOP_LEVEL ON)
endif()

option(HOME_EMS_BUILD_TESTS "Build the unit tests" ${HOME_EMS_TOP_LEVEL})
option(HOME_EMS_BUILD_EXAMPLES "Build the examples" ${HOME_EMS_TOP_LEVEL})
option(HOME_EMS_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)
option(HOME_EMS_ENABLE_ASAN "Build with AddressSanitizer" OFF)
option(HOME_EMS_ENABLE_UBSAN "Build with UndefinedBehaviorSanitizer" OFF)
option(HOME_EMS_ENABLE_COVERAGE "Build with coverage instrumentation" OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

include(cmake/ProjectOptions.cmake)

# Library ---------------------------------------------------------------------

add_library(home_ems
  src/battery_module.cc
  src/battery_storage.cc
  src/energy_controller.cc
  src/energy_system.cc
  src/grid.cc
  src/house.cc
  src/inverter.cc
  src/parse_number.cc
  src/profile.cc
  src/pv_system.cc
  src/simulation.cc)
add_library(home_ems::home_ems ALIAS home_ems)
target_include_directories(home_ems PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_features(home_ems PUBLIC cxx_std_17)
set_target_properties(home_ems PROPERTIES CXX_EXTENSIONS OFF)
# BUILD_INTERFACE keeps the warning and sanitizer flags out of the export.
target_link_libraries(home_ems PRIVATE $<BUILD_INTERFACE:home_ems_project_options>)

# Command-line app ------------------------------------------------------------

add_library(home_ems_cli_options STATIC apps/cli_options.cc)
target_include_directories(home_ems_cli_options PUBLIC ${PROJECT_SOURCE_DIR}/apps)
target_link_libraries(home_ems_cli_options
  PUBLIC home_ems
  PRIVATE home_ems_project_options)

add_executable(home_ems_cli apps/main.cc)
set_target_properties(home_ems_cli PROPERTIES OUTPUT_NAME home-ems)
target_link_libraries(home_ems_cli PRIVATE home_ems_cli_options home_ems_project_options)

# Examples and tests ----------------------------------------------------------

if(HOME_EMS_BUILD_EXAMPLES)
  add_executable(single_step examples/single_step.cc)
  target_link_libraries(single_step PRIVATE home_ems home_ems_project_options)
endif()

if(HOME_EMS_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

# Install ---------------------------------------------------------------------

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS home_ems EXPORT home_emsTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS home_ems_cli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/home_ems DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT home_emsTargets
  NAMESPACE home_ems::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/home_ems)

configure_package_config_file(cmake/home_emsConfig.cmake.in
  ${PROJECT_BINARY_DIR}/home_emsConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/home_ems)
write_basic_package_version_file(${PROJECT_BINARY_DIR}/home_emsConfigVersion.cmake
  COMPATIBILITY SameMinorVersion)
install(FILES
  ${PROJECT_BINARY_DIR}/home_emsConfig.cmake
  ${PROJECT_BINARY_DIR}/home_emsConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/home_ems)
