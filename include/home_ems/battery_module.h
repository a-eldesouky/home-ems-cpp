#ifndef HOME_EMS_BATTERY_MODULE_H_
#define HOME_EMS_BATTERY_MODULE_H_

#include "home_ems/units.h"

namespace home_ems {

// One battery module as reported by its battery management system (BMS).
class BatteryModule {
 public:
  static constexpr double kMinTemperatureC = -20.0;
  static constexpr double kMaxTemperatureC = 60.0;

  // Throws InvalidParameterError unless `voltage` and `max_power` are finite
  // and > 0 and `temperature` is in [kMinTemperatureC, kMaxTemperatureC].
  // NaN fails every check.
  BatteryModule(Temperature temperature, Voltage voltage, Power max_power);

  BatteryModule(const BatteryModule&) = default;
  BatteryModule& operator=(const BatteryModule&) = default;
  BatteryModule(BatteryModule&&) noexcept = default;
  BatteryModule& operator=(BatteryModule&&) noexcept = default;
  ~BatteryModule() = default;

  [[nodiscard]] Temperature temperature() const { return temperature_; }
  [[nodiscard]] Voltage voltage() const { return voltage_; }
  [[nodiscard]] Power max_power() const { return max_power_; }

 private:
  Temperature temperature_;
  Voltage voltage_;
  Power max_power_;
};

}  // namespace home_ems

#endif  // HOME_EMS_BATTERY_MODULE_H_
