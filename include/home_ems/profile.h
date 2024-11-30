#ifndef HOME_EMS_PROFILE_H_
#define HOME_EMS_PROFILE_H_

#include <istream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "home_ems/units.h"

namespace home_ems {

// Header line a profile CSV must start with.
inline constexpr std::string_view kProfileHeader = "time,pv_w,load_w";

// One row of a PV and load profile.
struct ProfileSample {
  std::string time;  // Free-form label, copied to the output.
  Power pv;          // PV output in watts, >= 0.
  Power load;        // House consumption in watts, >= 0.
  int line = 0;      // Line number in the input, for error messages.
};

// Reads a profile CSV: the kProfileHeader line, then one "time,pv_w,load_w"
// row per step. Blank lines and a trailing '\r' are ignored. Throws
// InvalidParameterError with the line number if the header is missing, a row
// does not have three fields, the time is empty, or a power value is not a
// finite number >= 0.
std::vector<ProfileSample> ReadProfile(std::istream& in);

// Returns minutes since midnight for a "HH:MM" label (00:00 to 23:59), or
// std::nullopt for any other text.
std::optional<int> ParseClockMinutes(const std::string& label);

// If the first sample's time is a "HH:MM" label, checks that every sample is
// "HH:MM" and exactly `step_minutes` after the previous one, wrapping at
// midnight. Free-form labels are not checked. Throws InvalidParameterError
// with the line number on a mismatch, or if `step_minutes` is not in
// [1, 1440].
void CheckTimeSpacing(const std::vector<ProfileSample>& samples,
                      int step_minutes);

}  // namespace home_ems

#endif  // HOME_EMS_PROFILE_H_
