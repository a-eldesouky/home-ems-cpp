#include "home_ems/inverter.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "home_ems/battery_module.h"
#include "home_ems/errors.h"
#include "home_ems/grid.h"
#include "home_ems/units.h"
#include "validation.h"

namespace home_ems {

Inverter::Inverter(Power max_power) : max_power_(max_power) {
  // A rating <= 0 is allowed here; ControlStep reports it as a failed step.
  internal::RequireFinite(max_power.value, "Inverter max power");
}

Voltage Inverter::BatteryVoltage(const std::vector<BatteryModule>& modules,
                                 const VoltageTolerance& tolerance) const {
  internal::RequireNonNegative(tolerance.absolute.value,
                               "Absolute voltage tolerance");
  internal::RequireNonNegative(tolerance.relative,
                               "Relative voltage tolerance");
  if (modules.empty()) {
    throw HardwareFailureError("No battery modules connected to inverter.");
  }
  const double first = modules.front().voltage().value;
  const double allowed =
      std::max(tolerance.absolute.value, tolerance.relative * std::abs(first));
  const bool consistent = std::all_of(
      modules.begin(), modules.end(), [first, allowed](const BatteryModule& m) {
        return std::abs(m.voltage().value - first) <= allowed;
      });
  if (!consistent) {
    throw HardwareFailureError("Inconsistent voltages across battery modules.");
  }
  Voltage sum(kZero);
  for (const BatteryModule& module : modules) {
    sum += module.voltage();
  }
  return Voltage(sum.value / static_cast<double>(modules.size()));
}

Current Inverter::BatteryCurrent(
    const std::vector<BatteryModule>& modules) const {
  if (modules.empty()) {
    throw HardwareFailureError("No battery modules connected to inverter.");
  }
  Current total(kZero);
  for (const BatteryModule& module : modules) {
    if (module.voltage().value == kZero) {
      throw HardwareFailureError("Battery module voltage is zero.");
    }
    total += Current(module.max_power().value / module.voltage().value);
  }
  return total;
}

Frequency Inverter::GridFrequency(const Grid& grid) const {
  return grid.frequency();
}

Voltage Inverter::GridVoltage(const Grid& grid) const { return grid.voltage(); }

void Inverter::SetPower(Power power) {
  internal::RequireFinite(power.value, "Inverter power");
  if (std::abs(power.value) > max_power_.value) {
    throw OverloadError("Power exceeds inverter's max capacity.");
  }
  power_ = power;
}

}  // namespace home_ems
