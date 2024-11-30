// Builds a system from the default configuration, runs one control step and
// prints what the controller decided.

#include <cstdlib>
#include <iostream>

#include "home_ems/energy_controller.h"
#include "home_ems/energy_system.h"
#include "home_ems/units.h"

int main() {
  home_ems::SystemConfig config;
  config.type = home_ems::SystemType::kBasic;
  home_ems::EnergySystem system(config);

  system.Update(home_ems::Power(6500.0), home_ems::Power(1500.0));
  const home_ems::ControlResult result = system.Run(home_ems::Hours(1.0));

  std::cout << "modules:         " << system.storage().modules().size() << '\n'
            << "inverter max W:  " << system.storage().inverter().max_power()
            << '\n'
            << "net W:           " << result.net << '\n'
            << "battery W:       " << result.battery_command << '\n'
            << "grid export W:   " << result.grid_export << '\n'
            << "grid import W:   " << result.grid_import << '\n';
  return result.error.empty() ? EXIT_SUCCESS : EXIT_FAILURE;
}
