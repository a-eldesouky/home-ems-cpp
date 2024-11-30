#ifndef TESTS_TEST_HELPERS_H_
#define TESTS_TEST_HELPERS_H_

#include <cstddef>
#include <vector>

#include "home_ems/battery_module.h"
#include "home_ems/battery_storage.h"
#include "home_ems/grid.h"
#include "home_ems/house.h"
#include "home_ems/inverter.h"
#include "home_ems/pv_system.h"
#include "home_ems/units.h"

namespace home_ems::testing {

// One-hour step, so energies in Wh equal powers in W numerically.
inline constexpr Hours kHour{1.0};

inline BatteryModule MakeModule(double voltage = 48.0) {
  return {Temperature(25.0), Voltage(voltage), Power(2000.0)};
}

// Three 2000 W modules by default (6000 W), so a 4000 W inverter is the
// limit, as in the default system.
inline BatteryStorage MakeStorage(double inverter_max_w = 4000.0,
                                  std::size_t module_count = 3) {
  return {Inverter(Power(inverter_max_w)),
          std::vector<BatteryModule>(module_count, MakeModule())};
}

inline PvSystem MakePv(double watts) { return {Power(watts), Voltage(220.0)}; }

inline House MakeHouse(double watts) {
  return {Power(watts), Voltage(220.0), Frequency(50.0)};
}

inline Grid MakeGrid() { return {Voltage(230.0), Frequency(50.0)}; }

}  // namespace home_ems::testing

#endif  // TESTS_TEST_HELPERS_H_
