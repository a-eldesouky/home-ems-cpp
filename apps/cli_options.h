#ifndef APPS_CLI_OPTIONS_H_
#define APPS_CLI_OPTIONS_H_

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace home_ems::cli {

// Default inverter rating in watts, the same as SystemConfig's default.
inline constexpr double kDefaultInverterMaxW = 4000.0;
inline constexpr int kDefaultStepMinutes = 60;
inline constexpr double kDefaultSocMinPct = 10.0;
inline constexpr double kDefaultSocMaxPct = 95.0;
inline constexpr double kDefaultSocInitialPct = 50.0;
inline constexpr double kDefaultEfficiency = 1.0;

struct CliOptions {
  std::string profile_path;
  double inverter_max_w = kDefaultInverterMaxW;
  int step_minutes = kDefaultStepMinutes;
  // Usable capacity of one battery module. Set to enable the state of charge
  // model; the --soc-* and --efficiency options need it.
  std::optional<double> capacity_wh;
  double soc_min_pct = kDefaultSocMinPct;
  double soc_max_pct = kDefaultSocMaxPct;
  double soc_initial_pct = kDefaultSocInitialPct;
  double efficiency = kDefaultEfficiency;  // One-way, charge and discharge.
  bool show_help = false;
};

// Parses the arguments after the program name. Throws InvalidParameterError
// for an unknown option, a missing or invalid value, inconsistent state of
// charge limits, a --soc-* or --efficiency option without --capacity-wh, or
// a missing or extra profile path. With --help, no other check is made.
CliOptions ParseCliOptions(const std::vector<std::string>& args);

// Returns the --help text.
std::string UsageText(std::string_view program);

}  // namespace home_ems::cli

#endif  // APPS_CLI_OPTIONS_H_
