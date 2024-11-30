#ifndef HOME_EMS_INVERTER_H_
#define HOME_EMS_INVERTER_H_

#include <vector>

#include "home_ems/battery_module.h"
#include "home_ems/grid.h"
#include "home_ems/units.h"

namespace home_ems {

// How far module voltages may differ from the first module's voltage and still
// count as the same. The allowed difference is max(absolute, relative * |v0|).
struct VoltageTolerance {
  Voltage absolute{0.5};
  double relative = 0.01;
};

// Battery inverter. Its rated power limits how fast the battery can charge or
// discharge.
class Inverter {
 public:
  // Throws InvalidParameterError if `max_power` is not finite. A rating <= 0
  // is accepted; ControlStep then reports every step as failed.
  explicit Inverter(Power max_power);

  Inverter(const Inverter&) = default;
  Inverter& operator=(const Inverter&) = default;
  Inverter(Inverter&&) noexcept = default;
  Inverter& operator=(Inverter&&) noexcept = default;
  ~Inverter() = default;

  [[nodiscard]] Power max_power() const { return max_power_; }
  // Last power set point. Positive charges, negative discharges.
  [[nodiscard]] Power power() const { return power_; }

  // Returns the mean voltage of `modules`. Throws HardwareFailureError if
  // `modules` is empty or a module differs from the first by more than
  // `tolerance` allows, and InvalidParameterError if a tolerance value is
  // negative or not finite.
  [[nodiscard]] Voltage BatteryVoltage(
      const std::vector<BatteryModule>& modules,
      const VoltageTolerance& tolerance = VoltageTolerance{}) const;

  // Returns the sum of max_power / voltage over `modules`. Throws
  // HardwareFailureError if `modules` is empty or a voltage is zero.
  [[nodiscard]] Current BatteryCurrent(
      const std::vector<BatteryModule>& modules) const;

  [[nodiscard]] Frequency GridFrequency(const Grid& grid) const;
  [[nodiscard]] Voltage GridVoltage(const Grid& grid) const;

  // Sets the power set point. Throws InvalidParameterError if `power` is not
  // finite and OverloadError if |power| > max_power(). On a throw the set
  // point is unchanged.
  void SetPower(Power power);

 private:
  Power max_power_;
  Power power_;
};

}  // namespace home_ems

#endif  // HOME_EMS_INVERTER_H_
