#ifndef SRC_VALIDATION_H_
#define SRC_VALIDATION_H_

#include <cmath>
#include <string>
#include <string_view>

#include "home_ems/errors.h"

namespace home_ems::internal {

// Throws InvalidParameterError unless `value` is finite and > 0. NaN fails.
inline void RequirePositive(double value, std::string_view what) {
  if (!(std::isfinite(value) && value > 0.0)) {
    throw InvalidParameterError(std::string(what) +
                                " must be a finite number > 0.");
  }
}

// Throws InvalidParameterError unless `value` is finite and >= 0. NaN fails.
inline void RequireNonNegative(double value, std::string_view what) {
  if (!(std::isfinite(value) && value >= 0.0)) {
    throw InvalidParameterError(std::string(what) +
                                " must be a finite number >= 0.");
  }
}

// Throws InvalidParameterError unless `value` is finite.
inline void RequireFinite(double value, std::string_view what) {
  if (!std::isfinite(value)) {
    throw InvalidParameterError(std::string(what) + " must be finite.");
  }
}

}  // namespace home_ems::internal

#endif  // SRC_VALIDATION_H_
