#include "cli_options.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "home_ems/errors.h"
#include "home_ems/parse_number.h"

namespace home_ems::cli {
namespace {

constexpr int kMinutesPerDay = 1440;
constexpr double kMaxPercent = 100.0;

[[noreturn]] void Reject(std::string_view option, std::string_view need,
                         const std::string& text) {
  throw InvalidParameterError(std::string(option) + " needs " +
                              std::string(need) + ", got '" + text + "'");
}

double ParseFinite(const std::string& text, std::string_view option,
                   std::string_view need) {
  const std::optional<double> value = ParseNumber(text);
  if (!value || !std::isfinite(*value)) {
    Reject(option, need, text);
  }
  return *value;
}

double ParsePositive(const std::string& text, std::string_view option) {
  const double value = ParseFinite(text, option, "a positive number");
  if (value <= 0.0) {
    Reject(option, "a positive number", text);
  }
  return value;
}

double ParsePercent(const std::string& text, std::string_view option) {
  const double value = ParseFinite(text, option, "a percentage from 0 to 100");
  if (value < 0.0 || value > kMaxPercent) {
    Reject(option, "a percentage from 0 to 100", text);
  }
  return value;
}

double ParseEfficiency(const std::string& text, std::string_view option) {
  const double value = ParseFinite(text, option, "a number in (0, 1]");
  if (value <= 0.0 || value > 1.0) {
    Reject(option, "a number in (0, 1]", text);
  }
  return value;
}

int ParseStepMinutes(const std::string& text, std::string_view option) {
  const double value =
      ParseFinite(text, option, "a whole number of minutes from 1 to 1440");
  if (value < 1.0 || value > kMinutesPerDay || std::floor(value) != value) {
    Reject(option, "a whole number of minutes from 1 to 1440", text);
  }
  return static_cast<int>(value);
}

void CheckSocOptions(const CliOptions& options, bool soc_option_given) {
  if (!options.capacity_wh) {
    if (soc_option_given) {
      throw InvalidParameterError(
          "--soc-min, --soc-max, --soc-initial and --efficiency need "
          "--capacity-wh");
    }
    return;
  }
  if (options.soc_min_pct >= options.soc_max_pct) {
    throw InvalidParameterError("--soc-min must be below --soc-max");
  }
  if (options.soc_initial_pct < options.soc_min_pct ||
      options.soc_initial_pct > options.soc_max_pct) {
    throw InvalidParameterError(
        "--soc-initial must be between --soc-min and --soc-max");
  }
}

}  // namespace

CliOptions ParseCliOptions(const std::vector<std::string>& args) {
  CliOptions options;
  if (std::find(args.begin(), args.end(), "--help") != args.end() ||
      std::find(args.begin(), args.end(), "-h") != args.end()) {
    options.show_help = true;
    return options;
  }

  bool soc_option_given = false;
  std::vector<std::string> positional;
  for (std::size_t i = 0; i < args.size(); ++i) {
    const std::string& arg = args[i];
    const bool takes_value =
        arg == "--inverter-max-w" || arg == "--step-minutes" ||
        arg == "--capacity-wh" || arg == "--soc-min" || arg == "--soc-max" ||
        arg == "--soc-initial" || arg == "--efficiency";
    if (takes_value) {
      if (i + 1 == args.size()) {
        throw InvalidParameterError(arg + " needs a value");
      }
      ++i;
      const std::string& value = args[i];
      if (arg == "--inverter-max-w") {
        options.inverter_max_w = ParsePositive(value, arg);
      } else if (arg == "--step-minutes") {
        options.step_minutes = ParseStepMinutes(value, arg);
      } else if (arg == "--capacity-wh") {
        options.capacity_wh = ParsePositive(value, arg);
      } else {
        soc_option_given = true;
        if (arg == "--soc-min") {
          options.soc_min_pct = ParsePercent(value, arg);
        } else if (arg == "--soc-max") {
          options.soc_max_pct = ParsePercent(value, arg);
        } else if (arg == "--soc-initial") {
          options.soc_initial_pct = ParsePercent(value, arg);
        } else {
          options.efficiency = ParseEfficiency(value, arg);
        }
      }
    } else if (arg.size() > 1 && arg.front() == '-') {
      throw InvalidParameterError("unknown option '" + arg + "'");
    } else {
      positional.push_back(arg);
    }
  }

  if (positional.size() != 1) {
    throw InvalidParameterError("expected one profile CSV path, got " +
                                std::to_string(positional.size()));
  }
  options.profile_path = positional.front();
  CheckSocOptions(options, soc_option_given);
  return options;
}

std::string UsageText(std::string_view program) {
  return "Usage: " + std::string(program) +
         " [OPTIONS] PROFILE.csv\n"
         "\n"
         "Runs the battery dispatch rule once per row of PROFILE.csv\n"
         "(header: time,pv_w,load_w), prints one CSV row per step on stdout\n"
         "and energy totals on stderr.\n"
         "\n"
         "Options:\n"
         "  --inverter-max-w WATTS  Inverter power limit (default 4000)\n"
         "  --step-minutes MIN      Length of one row, 1 to 1440 (default 60)\n"
         "  --capacity-wh WH        Usable capacity of one battery module;\n"
         "                          enables the state of charge model\n"
         "  --soc-min PCT           Lowest state of charge (default 10)\n"
         "  --soc-max PCT           Highest state of charge (default 95)\n"
         "  --soc-initial PCT       State of charge at the start (default 50)\n"
         "  --efficiency E          One-way charge and discharge efficiency,\n"
         "                          in (0, 1] (default 1)\n"
         "  -h, --help              Show this text\n";
}

}  // namespace home_ems::cli
