#ifndef HOME_EMS_HOUSE_H_
#define HOME_EMS_HOUSE_H_

#include "home_ems/units.h"

namespace home_ems {

// Household load. The current is derived as I = P / V.
class House {
 public:
  // Throws InvalidParameterError unless `power` is finite and >= 0 and
  // `voltage` and `frequency` are finite and > 0.
  House(Power power, Voltage voltage, Frequency frequency);

  House(const House&) = default;
  House& operator=(const House&) = default;
  House(House&&) noexcept = default;
  House& operator=(House&&) noexcept = default;
  ~House() = default;

  // Sets a new consumption and recomputes the current. Throws
  // InvalidParameterError unless `power` is finite and >= 0; the state is
  // unchanged on a throw.
  void SetPowerConsumption(Power power);

  [[nodiscard]] Power power_consumption() const { return power_; }
  [[nodiscard]] Voltage voltage() const { return voltage_; }
  [[nodiscard]] Current current() const { return current_; }
  [[nodiscard]] Frequency frequency() const { return frequency_; }

 private:
  Power power_;
  Voltage voltage_;
  Frequency frequency_;
  Current current_;
};

}  // namespace home_ems

#endif  // HOME_EMS_HOUSE_H_
