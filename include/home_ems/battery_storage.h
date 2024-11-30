#ifndef HOME_EMS_BATTERY_STORAGE_H_
#define HOME_EMS_BATTERY_STORAGE_H_

#include <optional>
#include <vector>

#include "home_ems/battery_module.h"
#include "home_ems/inverter.h"
#include "home_ems/units.h"

namespace home_ems {

// Optional energy model for a storage unit. Without it the battery never
// fills up or runs empty. State of charge values are fractions in [0, 1].
struct StateOfChargeModel {
  Energy module_capacity{5000.0};     // Usable capacity of one module, > 0.
  double min_soc = 0.10;              // Lowest allowed state of charge.
  double max_soc = 0.95;              // Highest allowed state of charge.
  double initial_soc = 0.50;          // State of charge before the first step.
  double charge_efficiency = 1.0;     // Stored / drawn energy, in (0, 1].
  double discharge_efficiency = 1.0;  // Delivered / removed energy, in (0, 1].
};

// Battery storage unit: one inverter and one or more battery modules. Owns
// both by value.
class BatteryStorage {
 public:
  // Throws HardwareFailureError if `modules` is empty and
  // InvalidParameterError if `soc_model` is given and not valid: capacity
  // finite and > 0, 0 <= min_soc < max_soc <= 1, initial_soc in
  // [min_soc, max_soc], efficiencies in (0, 1].
  BatteryStorage(Inverter inverter, std::vector<BatteryModule> modules,
                 std::optional<StateOfChargeModel> soc_model = std::nullopt);

  BatteryStorage(const BatteryStorage&) = default;
  BatteryStorage& operator=(const BatteryStorage&) = default;
  BatteryStorage(BatteryStorage&&) noexcept = default;
  BatteryStorage& operator=(BatteryStorage&&) noexcept = default;
  ~BatteryStorage() = default;

  // Runs the battery at `power` (negated for kDischarge) for `step`: sends it
  // to the inverter, stores it as the command and, with an energy model,
  // updates the stored energy. Throws InvalidParameterError if `power` is not
  // finite or `step` is not finite and > 0, and OverloadError if the power is
  // above MaxChargePower(step) or MaxDischargePower(step). On a throw nothing
  // changes.
  void SetPowerCommand(PowerCommand command, Power power, Hours step);

  // Smaller of the inverter rating and the sum of the module ratings.
  [[nodiscard]] Power max_power() const;
  // Sum of the module ratings.
  [[nodiscard]] Power module_max_power() const { return module_max_power_; }

  // Highest charge power for one step of length `step`: max_power(), further
  // limited by the room left below max_soc when there is an energy model.
  // Throws InvalidParameterError if `step` is not finite and > 0.
  [[nodiscard]] Power MaxChargePower(Hours step) const;
  // Highest discharge power for one step of length `step`: max_power(),
  // further limited by the energy left above min_soc when there is an energy
  // model. Throws InvalidParameterError if `step` is not finite and > 0.
  [[nodiscard]] Power MaxDischargePower(Hours step) const;

  // Last command. Positive charges, negative discharges.
  [[nodiscard]] Power power_command() const { return power_command_; }
  [[nodiscard]] const Inverter& inverter() const { return inverter_; }
  [[nodiscard]] const std::vector<BatteryModule>& modules() const {
    return modules_;
  }

  // Energy model values; std::nullopt without an energy model.
  [[nodiscard]] std::optional<Energy> capacity() const;
  [[nodiscard]] std::optional<Energy> stored_energy() const;
  [[nodiscard]] std::optional<double> state_of_charge() const;

 private:
  Power power_command_;
  Inverter inverter_;
  std::vector<BatteryModule> modules_;
  Power module_max_power_;
  std::optional<StateOfChargeModel> soc_model_;
  Energy capacity_;
  // min_soc * capacity_ and max_soc * capacity_, computed once so that the
  // limits and the clamp use the same values. Recomputing them as part of a
  // longer expression lets the compiler fuse the multiply into an FMA, which
  // leaves a rounding residue (about 1e-13) when the battery is full or empty.
  Energy min_energy_;
  Energy max_energy_;
  Energy stored_energy_;
};

}  // namespace home_ems

#endif  // HOME_EMS_BATTERY_STORAGE_H_
