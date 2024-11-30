#include "home_ems/energy_controller.h"

#include <algorithm>
#include <cmath>
#include <exception>

#include "home_ems/battery_storage.h"
#include "home_ems/errors.h"
#include "home_ems/grid.h"
#include "home_ems/house.h"
#include "home_ems/pv_system.h"
#include "home_ems/units.h"

namespace home_ems {
namespace {

// Charges the battery with as much of `surplus` as the storage allows and
// sells the rest. Returns the power sold.
Power HandleSurplus(Power surplus, Power rating, Hours step,
                    BatteryStorage& storage, Grid& grid) {
  if (!(rating.value > kZero)) {
    throw HardwareFailureError("Invalid storage max charge power.");
  }
  const Power limit = storage.MaxChargePower(step);
  const Power charge_power(std::min(surplus.value, limit.value));
  storage.SetPowerCommand(PowerCommand::kCharge, charge_power, step);

  const Power power_to_grid = surplus - charge_power;
  if (!power_to_grid.IsRemaining()) {
    return Power(kZero);
  }
  grid.SellEnergy(power_to_grid * step);
  return power_to_grid;
}

// Discharges the battery with as much of `shortage` as the storage allows
// and buys the rest. Returns the power bought.
Power HandleShortage(Power shortage, Power rating, Hours step,
                     BatteryStorage& storage, Grid& grid) {
  if (!(rating.value > kZero)) {
    throw HardwareFailureError("Invalid storage max discharge power.");
  }
  const Power limit = storage.MaxDischargePower(step);
  const Power discharge_power(std::min(shortage.value, limit.value));
  storage.SetPowerCommand(PowerCommand::kDischarge, discharge_power, step);

  const Power power_from_grid = shortage - discharge_power;
  if (!power_from_grid.IsRemaining()) {
    return Power(kZero);
  }
  grid.BuyEnergy(power_from_grid * step);
  return power_from_grid;
}

}  // namespace

ControlResult ControlStep(const PvSystem& pv_system, const House& house,
                          BatteryStorage& storage, Grid& grid, Hours step) {
  ControlResult result;
  result.net = pv_system.power() - house.power_consumption();
  const Power rating = storage.max_power();

  try {
    if (!(std::isfinite(step.value) && step.value > kZero)) {
      throw InvalidParameterError("Step length must be a finite number > 0.");
    }
    if (result.net.IsSurplus()) {
      result.grid_export =
          HandleSurplus(result.net, rating, step, storage, grid);
    } else {
      const Power shortage(-result.net.value);
      result.grid_import =
          HandleShortage(shortage, rating, step, storage, grid);
    }
    result.battery_command = storage.power_command();
  } catch (const std::exception& e) {
    // A failed step leaves the system running; the caller sees the reason.
    // The battery did not run in this step, so battery_command stays 0 and
    // does not repeat the previous step's command.
    result.error = e.what();
  }
  result.state_of_charge = storage.state_of_charge();
  return result;
}

}  // namespace home_ems
