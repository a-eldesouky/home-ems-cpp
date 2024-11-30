#ifndef HOME_EMS_PARSE_NUMBER_H_
#define HOME_EMS_PARSE_NUMBER_H_

#include <optional>
#include <string>

namespace home_ems {

// Parses `text` as a decimal floating-point number. The whole string must be
// the number: no leading or trailing whitespace, no hexadecimal, no embedded
// NUL. "inf" and "nan" are returned as values, so callers that need a finite
// number must check with std::isfinite. A value too small to represent
// becomes 0 or a subnormal; a value too large returns std::nullopt.
std::optional<double> ParseNumber(const std::string& text);

}  // namespace home_ems

#endif  // HOME_EMS_PARSE_NUMBER_H_
