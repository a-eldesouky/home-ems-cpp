#include "home_ems/battery_module.h"

#include <cmath>

#include "home_ems/errors.h"
#include "home_ems/units.h"
#include "validation.h"

namespace home_ems {

BatteryModule::BatteryModule(Temperature temperature, Voltage voltage,
                             Power max_power)
    : temperature_(temperature), voltage_(voltage), max_power_(max_power) {
  internal::RequirePositive(voltage.value, "Battery module voltage");
  internal::RequirePositive(max_power.value, "Battery module max power");
  // Written so that NaN fails the check.
  if (!(temperature.value >= kMinTemperatureC &&
        temperature.value <= kMaxTemperatureC)) {
    throw InvalidParameterError("Battery module temperature out of range.");
  }
}

}  // namespace home_ems
