#include "home_ems/simulation.h"

#include <iomanip>
#include <ios>
#include <optional>
#include <ostream>
#include <vector>

#include "home_ems/energy_system.h"
#include "home_ems/profile.h"
#include "home_ems/units.h"

namespace home_ems {
namespace {

constexpr double kPercent = 100.0;

// Maps -0.0 to 0.0 so a zero value prints as "0.0", not "-0.0".
double Printable(double value) { return value == kZero ? kZero : value; }

// Saves and restores a stream's number format.
class FormatGuard {
 public:
  explicit FormatGuard(std::ostream& out)
      : out_(out), flags_(out.flags()), precision_(out.precision()) {}
  FormatGuard(const FormatGuard&) = delete;
  FormatGuard& operator=(const FormatGuard&) = delete;
  FormatGuard(FormatGuard&&) = delete;
  FormatGuard& operator=(FormatGuard&&) = delete;
  ~FormatGuard() {
    out_.flags(flags_);
    out_.precision(precision_);
  }

 private:
  std::ostream& out_;
  std::ios_base::fmtflags flags_;
  std::streamsize precision_;
};

}  // namespace

std::vector<StepRecord> Simulate(EnergySystem& system,
                                 const std::vector<ProfileSample>& samples,
                                 Hours step) {
  std::vector<StepRecord> records;
  records.reserve(samples.size());
  for (const ProfileSample& sample : samples) {
    system.Update(sample.pv, sample.load);
    records.push_back({sample.time, sample.pv, sample.load, system.Run(step)});
  }
  return records;
}

SimulationSummary Summarize(const std::vector<StepRecord>& records,
                            Hours step) {
  SimulationSummary summary;
  summary.steps = records.size();
  summary.step = step;
  for (const StepRecord& r : records) {
    if (!r.result.error.empty()) {
      ++summary.failed_steps;
      continue;
    }
    summary.grid_export += r.result.grid_export * step;
    summary.grid_import += r.result.grid_import * step;
    const Power battery = r.result.battery_command;
    if (battery.value > kZero) {
      summary.battery_charge += battery * step;
    } else {
      summary.battery_discharge += Power(-battery.value) * step;
    }
  }
  if (!records.empty()) {
    summary.final_state_of_charge = records.back().result.state_of_charge;
  }
  return summary;
}

void WriteResultsCsv(std::ostream& out,
                     const std::vector<StepRecord>& records) {
  const FormatGuard guard(out);
  out << "time,pv_w,load_w,net_w,battery_w,grid_export_w,grid_import_w,"
         "soc_pct,error\n";
  out << std::fixed << std::setprecision(1);
  for (const StepRecord& r : records) {
    out << r.time << ',' << Printable(r.pv.value) << ','
        << Printable(r.load.value) << ',' << Printable(r.result.net.value)
        << ',' << Printable(r.result.battery_command.value) << ','
        << Printable(r.result.grid_export.value) << ','
        << Printable(r.result.grid_import.value) << ',';
    if (r.result.state_of_charge) {
      out << Printable(*r.result.state_of_charge * kPercent);
    }
    out << ',' << r.result.error << '\n';
  }
}

void WriteSummary(std::ostream& out, const SimulationSummary& summary) {
  const FormatGuard guard(out);
  out << std::fixed << std::setprecision(1);
  out << "steps: " << summary.steps << " (failed: " << summary.failed_steps
      << ")\n"
      << "step length: " << summary.step.value * 60.0 << " min\n"
      << "grid export: " << Printable(summary.grid_export.value) << " Wh\n"
      << "grid import: " << Printable(summary.grid_import.value) << " Wh\n"
      << "battery charge: " << Printable(summary.battery_charge.value)
      << " Wh\n"
      << "battery discharge: " << Printable(summary.battery_discharge.value)
      << " Wh\n";
  if (summary.final_state_of_charge) {
    out << "final state of charge: "
        << Printable(*summary.final_state_of_charge * kPercent) << " %\n";
  }
}

}  // namespace home_ems
