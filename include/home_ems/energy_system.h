#ifndef HOME_EMS_ENERGY_SYSTEM_H_
#define HOME_EMS_ENERGY_SYSTEM_H_

#include <optional>

#include "home_ems/battery_storage.h"
#include "home_ems/energy_controller.h"
#include "home_ems/grid.h"
#include "home_ems/house.h"
#include "home_ems/pv_system.h"
#include "home_ems/units.h"

namespace home_ems {

// Parameters for building a system from defaults.
struct SystemConfig {
  SystemType type = SystemType::kStandard;
  Voltage grid_voltage{230.0};
  Frequency grid_frequency{50.0};
  Power pv_power{5000.0};
  Voltage pv_voltage{220.0};
  Power house_power{3000.0};
  Voltage house_voltage{220.0};
  Frequency house_frequency{50.0};
  Temperature module_temperature{25.0};
  Voltage module_voltage{48.0};
  Power module_max_power{2000.0};
  Power inverter_max_power{4000.0};
  // Energy model for the storage; none by default (the battery never fills
  // up or runs empty).
  std::optional<StateOfChargeModel> soc_model;
};

// A complete home system: PV, grid, house and battery storage. Owns all
// components by value.
class EnergySystem {
 public:
  // Builds every component from `config`, with ModuleCount(config.type)
  // identical battery modules. Throws if a component rejects its parameters.
  explicit EnergySystem(const SystemConfig& config);

  // Uses the given components. Throws InvalidParameterError if `storage` has
  // more than ModuleCount(type) battery modules.
  EnergySystem(SystemType type, PvSystem pv_system, Grid grid, House house,
               BatteryStorage storage);

  EnergySystem(const EnergySystem&) = default;
  EnergySystem& operator=(const EnergySystem&) = default;
  EnergySystem(EnergySystem&&) noexcept = default;
  EnergySystem& operator=(EnergySystem&&) noexcept = default;
  ~EnergySystem() = default;

  // Sets the PV output and house consumption for the next Run().
  void Update(Power pv_power, Power house_power);

  // Runs one control step of length `step` on the current state.
  ControlResult Run(Hours step);

  [[nodiscard]] SystemType type() const { return type_; }
  [[nodiscard]] const PvSystem& pv_system() const { return pv_system_; }
  [[nodiscard]] const Grid& grid() const { return grid_; }
  [[nodiscard]] const House& house() const { return house_; }
  [[nodiscard]] const BatteryStorage& storage() const { return storage_; }

 private:
  SystemType type_;
  PvSystem pv_system_;
  Grid grid_;
  House house_;
  BatteryStorage storage_;
};

}  // namespace home_ems

#endif  // HOME_EMS_ENERGY_SYSTEM_H_
