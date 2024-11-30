#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "cli_options.h"
#include "home_ems/energy_system.h"
#include "home_ems/profile.h"
#include "home_ems/simulation.h"
#include "home_ems/units.h"

namespace {

constexpr int kExitUsage = 2;
constexpr double kPercent = 100.0;
constexpr double kMinutesPerHour = 60.0;

int Run(const home_ems::cli::CliOptions& options) {
  std::ifstream file(options.profile_path);
  if (!file) {
    std::cerr << "error: cannot open '" << options.profile_path << "'\n";
    return EXIT_FAILURE;
  }
  const std::vector<home_ems::ProfileSample> samples =
      home_ems::ReadProfile(file);
  if (samples.empty()) {
    std::cerr << "error: '" << options.profile_path << "' has no data rows\n";
    return EXIT_FAILURE;
  }

  home_ems::CheckTimeSpacing(samples, options.step_minutes);

  home_ems::SystemConfig config;
  config.inverter_max_power = home_ems::Power(options.inverter_max_w);
  if (options.capacity_wh) {
    home_ems::StateOfChargeModel model;
    model.module_capacity = home_ems::Energy(*options.capacity_wh);
    model.min_soc = options.soc_min_pct / kPercent;
    model.max_soc = options.soc_max_pct / kPercent;
    model.initial_soc = options.soc_initial_pct / kPercent;
    model.charge_efficiency = options.efficiency;
    model.discharge_efficiency = options.efficiency;
    config.soc_model = model;
  }
  home_ems::EnergySystem system(config);

  const home_ems::Hours step(options.step_minutes / kMinutesPerHour);
  const std::vector<home_ems::StepRecord> records =
      home_ems::Simulate(system, samples, step);
  home_ems::WriteResultsCsv(std::cout, records);
  home_ems::WriteSummary(std::cerr, home_ems::Summarize(records, step));
  return EXIT_SUCCESS;
}

}  // namespace

int main(int argc, char* argv[]) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic): argv.
  const std::vector<std::string> args(argv + 1, argv + argc);
  home_ems::cli::CliOptions options;
  try {
    options = home_ems::cli::ParseCliOptions(args);
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << "\n"
              << "Try 'home-ems --help'.\n";
    return kExitUsage;
  }
  if (options.show_help) {
    std::cout << home_ems::cli::UsageText("home-ems");
    return EXIT_SUCCESS;
  }

  try {
    return Run(options);
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << "\n";
    return EXIT_FAILURE;
  }
}
