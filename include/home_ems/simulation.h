#ifndef HOME_EMS_SIMULATION_H_
#define HOME_EMS_SIMULATION_H_

#include <cstddef>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "home_ems/energy_controller.h"
#include "home_ems/energy_system.h"
#include "home_ems/profile.h"
#include "home_ems/units.h"

namespace home_ems {

// One simulated step: the input sample and the controller's result.
struct StepRecord {
  std::string time;
  Power pv;
  Power load;
  ControlResult result;
};

// Energy totals over a run.
struct SimulationSummary {
  std::size_t steps = 0;
  std::size_t failed_steps = 0;
  Hours step;
  Energy grid_export;
  Energy grid_import;
  Energy battery_charge;     // Energy drawn to charge the battery.
  Energy battery_discharge;  // Energy delivered by the battery.
  std::optional<double> final_state_of_charge;
};

// Feeds each sample to `system` (Update, then Run with `step`) in order.
std::vector<StepRecord> Simulate(EnergySystem& system,
                                 const std::vector<ProfileSample>& samples,
                                 Hours step);

// Sums the energy flows in `records`, each step lasting `step`.
SimulationSummary Summarize(const std::vector<StepRecord>& records, Hours step);

// Writes `records` as CSV with the header
// "time,pv_w,load_w,net_w,battery_w,grid_export_w,grid_import_w,soc_pct,error".
// Values are printed with one decimal; -0.0 is printed as 0.0. soc_pct is
// empty without an energy model.
void WriteResultsCsv(std::ostream& out, const std::vector<StepRecord>& records);

// Writes `summary` as "key: value" lines, energies in Wh with one decimal.
void WriteSummary(std::ostream& out, const SimulationSummary& summary);

}  // namespace home_ems

#endif  // HOME_EMS_SIMULATION_H_
