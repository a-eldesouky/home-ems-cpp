#include "home_ems/energy_system.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "home_ems/battery_module.h"
#include "home_ems/battery_storage.h"
#include "home_ems/energy_controller.h"
#include "home_ems/errors.h"
#include "home_ems/grid.h"
#include "home_ems/house.h"
#include "home_ems/inverter.h"
#include "home_ems/pv_system.h"
#include "home_ems/units.h"

namespace home_ems {
namespace {

BatteryStorage MakeStorage(const SystemConfig& config) {
  const BatteryModule module(config.module_temperature, config.module_voltage,
                             config.module_max_power);
  std::vector<BatteryModule> modules(
      static_cast<std::size_t>(ModuleCount(config.type)), module);
  return {Inverter(config.inverter_max_power), std::move(modules),
          config.soc_model};
}

}  // namespace

EnergySystem::EnergySystem(const SystemConfig& config)
    : type_(config.type),
      pv_system_(config.pv_power, config.pv_voltage),
      grid_(config.grid_voltage, config.grid_frequency),
      house_(config.house_power, config.house_voltage, config.house_frequency),
      storage_(MakeStorage(config)) {}

EnergySystem::EnergySystem(SystemType type, PvSystem pv_system, Grid grid,
                           House house, BatteryStorage storage)
    : type_(type),
      pv_system_(pv_system),
      grid_(grid),
      house_(house),
      storage_(std::move(storage)) {
  const std::size_t allowed = static_cast<std::size_t>(ModuleCount(type));
  if (storage_.modules().size() > allowed) {
    throw InvalidParameterError(
        "System type allows at most " + std::to_string(allowed) +
        " battery modules, got " + std::to_string(storage_.modules().size()) +
        ".");
  }
}

void EnergySystem::Update(Power pv_power, Power house_power) {
  pv_system_.SetPower(pv_power);
  house_.SetPowerConsumption(house_power);
}

ControlResult EnergySystem::Run(Hours step) {
  return ControlStep(pv_system_, house_, storage_, grid_, step);
}

}  // namespace home_ems
