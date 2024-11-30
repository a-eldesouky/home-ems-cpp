include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
  URL_HASH SHA256=7b42b4d6ed48810c5362c265a17faebe90dc2373c885e5216439d37927f02926
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Treat GoogleTest headers as system headers so our strict warnings do not
# apply to them.
foreach(target gtest gtest_main)
  get_target_property(dirs ${target} INTERFACE_INCLUDE_DIRECTORIES)
  set_target_properties(${target} PROPERTIES
    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${dirs}")
endforeach()

include(GoogleTest)

add_executable(home_ems_tests
  battery_module_test.cc
  battery_storage_test.cc
  cli_options_test.cc
  energy_controller_test.cc
  energy_system_test.cc
  grid_test.cc
  house_pv_test.cc
  inverter_test.cc
  parse_number_test.cc
  profile_test.cc
  simulation_test.cc
  units_test.cc)
target_link_libraries(home_ems_tests PRIVATE
  home_ems home_ems_cli_options home_ems_project_options GTest::gtest_main)
gtest_discover_tests(home_ems_tests DISCOVERY_MODE PRE_TEST)

# End-to-end checks of the command-line app. Failure cases match the error
# text, so a crash does not count as a pass.
set(sample ${PROJECT_SOURCE_DIR}/data/sample_day.csv)
add_test(NAME cli.sample_day COMMAND home_ems_cli ${sample})
set_tests_properties(cli.sample_day PROPERTIES
  PASS_REGULAR_EXPRESSION "^time,pv_w,load_w,net_w,battery_w")
add_test(NAME cli.help COMMAND home_ems_cli --help)
set_tests_properties(cli.help PROPERTIES PASS_REGULAR_EXPRESSION "Usage:")
add_test(NAME cli.missing_file COMMAND home_ems_cli does_not_exist.csv)
set_tests_properties(cli.missing_file PROPERTIES
  PASS_REGULAR_EXPRESSION "error: cannot open 'does_not_exist.csv'")
add_test(NAME cli.bad_option COMMAND home_ems_cli --inverter-max-w -5 ${sample})
set_tests_properties(cli.bad_option PROPERTIES
  PASS_REGULAR_EXPRESSION "error: --inverter-max-w needs a positive number")
add_test(NAME cli.no_args COMMAND home_ems_cli)
set_tests_properties(cli.no_args PROPERTIES
  PASS_REGULAR_EXPRESSION "error: expected one profile CSV path, got 0")
add_test(NAME cli.header_only
  COMMAND home_ems_cli ${CMAKE_CURRENT_SOURCE_DIR}/data/header_only.csv)
set_tests_properties(cli.header_only PROPERTIES
  PASS_REGULAR_EXPRESSION "has no data rows")
add_test(NAME cli.bad_value
  COMMAND home_ems_cli ${CMAKE_CURRENT_SOURCE_DIR}/data/bad_value.csv)
set_tests_properties(cli.bad_value PROPERTIES
  PASS_REGULAR_EXPRESSION "error: line 3: pv_w is not a number: 'abc'")
add_test(NAME cli.summary COMMAND home_ems_cli ${sample})
set_tests_properties(cli.summary PROPERTIES
  PASS_REGULAR_EXPRESSION "grid export: [0-9.]+ Wh")
add_test(NAME cli.state_of_charge
  COMMAND home_ems_cli --capacity-wh 2000 ${sample})
set_tests_properties(cli.state_of_charge PROPERTIES
  PASS_REGULAR_EXPRESSION "final state of charge: [0-9.]+ %")
add_test(NAME cli.step_mismatch
  COMMAND home_ems_cli --step-minutes 30 ${sample})
set_tests_properties(cli.step_mismatch PROPERTIES
  PASS_REGULAR_EXPRESSION "error: line 3: time '01:00' is not 30 minutes after '00:00'")
add_test(NAME cli.soc_without_capacity
  COMMAND home_ems_cli --soc-min 20 ${sample})
set_tests_properties(cli.soc_without_capacity PROPERTIES
  PASS_REGULAR_EXPRESSION "need --capacity-wh")
if(HOME_EMS_BUILD_EXAMPLES)
  add_test(NAME example.single_step COMMAND single_step)
endif()
