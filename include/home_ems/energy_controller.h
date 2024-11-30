#ifndef HOME_EMS_ENERGY_CONTROLLER_H_
#define HOME_EMS_ENERGY_CONTROLLER_H_

#include <optional>
#include <string>

#include "home_ems/battery_storage.h"
#include "home_ems/grid.h"
#include "home_ems/house.h"
#include "home_ems/pv_system.h"
#include "home_ems/units.h"

namespace home_ems {

// Outcome of one control step.
struct ControlResult {
  // PV power minus house consumption. Positive is a surplus.
  Power net;
  // Storage command for this step. Positive charges, negative discharges;
  // 0 when the step failed.
  Power battery_command;
  // Power sold to the grid in this step.
  Power grid_export;
  // Power bought from the grid in this step.
  Power grid_import;
  // State of charge after the step, as a fraction; empty without an energy
  // model.
  std::optional<double> state_of_charge;
  // Empty on success. Otherwise the reason the step was aborted; the step
  // does not throw.
  std::string error;
};

// Runs one control step of length `step`:
//   net = PV power - house consumption
//   net > 0:  charge with min(net, storage.MaxChargePower(step)) and sell the
//             rest to the grid.
//   net <= 0: discharge with min(-net, storage.MaxDischargePower(step)) and
//             buy the rest.
// Grid totals grow by power * step. A storage rating (storage.max_power())
// <= 0, or a step that is not finite and > 0, aborts the step and sets
// ControlResult::error; nothing changes then.
ControlResult ControlStep(const PvSystem& pv_system, const House& house,
                          BatteryStorage& storage, Grid& grid, Hours step);

}  // namespace home_ems

#endif  // HOME_EMS_ENERGY_CONTROLLER_H_
