#ifndef HOME_EMS_PV_SYSTEM_H_
#define HOME_EMS_PV_SYSTEM_H_

#include "home_ems/units.h"

namespace home_ems {

// Photovoltaic generator. The current is derived as I = P / V.
class PvSystem {
 public:
  // Throws InvalidParameterError unless `power` is finite and >= 0 and
  // `voltage` is finite and > 0.
  PvSystem(Power power, Voltage voltage);

  PvSystem(const PvSystem&) = default;
  PvSystem& operator=(const PvSystem&) = default;
  PvSystem(PvSystem&&) noexcept = default;
  PvSystem& operator=(PvSystem&&) noexcept = default;
  ~PvSystem() = default;

  // Sets a new output power and recomputes the current. Throws
  // InvalidParameterError unless `power` is finite and >= 0; the state is
  // unchanged on a throw.
  void SetPower(Power power);

  [[nodiscard]] Power power() const { return power_; }
  [[nodiscard]] Voltage voltage() const { return voltage_; }
  [[nodiscard]] Current current() const { return current_; }

 private:
  Power power_;
  Voltage voltage_;
  Current current_;
};

}  // namespace home_ems

#endif  // HOME_EMS_PV_SYSTEM_H_
