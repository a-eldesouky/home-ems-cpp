# Compile and link options for this project's own targets only. Third-party
# code (GoogleTest) does not link this target, so it does not get -Werror.

add_library(home_ems_project_options INTERFACE)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(home_ems_project_options INTERFACE
    -Wall -Wextra -Wpedantic
    -Wshadow -Wconversion -Wsign-conversion -Wold-style-cast
    -Wnon-virtual-dtor -Woverloaded-virtual -Wnull-dereference)
  if(HOME_EMS_WARNINGS_AS_ERRORS)
    target_compile_options(home_ems_project_options INTERFACE -Werror)
  endif()
elseif(MSVC)
  target_compile_options(home_ems_project_options INTERFACE /W4 /permissive-)
  if(HOME_EMS_WARNINGS_AS_ERRORS)
    target_compile_options(home_ems_project_options INTERFACE /WX)
  endif()
endif()

set(_home_ems_sanitizers "")
if(HOME_EMS_ENABLE_ASAN)
  list(APPEND _home_ems_sanitizers address)
endif()
if(HOME_EMS_ENABLE_UBSAN)
  list(APPEND _home_ems_sanitizers undefined)
endif()
if(_home_ems_sanitizers)
  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(FATAL_ERROR "Sanitizers need GCC or Clang")
  endif()
  list(JOIN _home_ems_sanitizers "," _home_ems_sanitizer_list)
  target_compile_options(home_ems_project_options INTERFACE
    -fsanitize=${_home_ems_sanitizer_list} -fno-omit-frame-pointer
    -fno-sanitize-recover=all)
  target_link_options(home_ems_project_options INTERFACE
    -fsanitize=${_home_ems_sanitizer_list})
endif()

if(HOME_EMS_ENABLE_COVERAGE)
  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(FATAL_ERROR "Coverage needs GCC or Clang")
  endif()
  target_compile_options(home_ems_project_options INTERFACE --coverage -O0 -g)
  target_link_options(home_ems_project_options INTERFACE --coverage)
endif()
