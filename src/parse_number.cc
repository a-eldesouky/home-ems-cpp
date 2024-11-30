#include "home_ems/parse_number.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <optional>
#include <string>

namespace home_ems {
namespace {

// strtod skips leading whitespace and accepts hexadecimal; both are rejected
// here so that every accepted string is a plain decimal number.
bool HasRejectedChar(const std::string& text) {
  return std::any_of(text.begin(), text.end(), [](char c) {
    return c == '\0' || c == 'x' || c == 'X' ||
           std::isspace(static_cast<unsigned char>(c)) != 0;
  });
}

}  // namespace

std::optional<double> ParseNumber(const std::string& text) {
  if (text.empty() || HasRejectedChar(text)) {
    return std::nullopt;
  }
  errno = 0;
  // strtod reports the end through a char**, so `end` cannot point to const.
  char* end = nullptr;  // NOLINT(misc-const-correctness)
  const double value = std::strtod(text.c_str(), &end);
  if (end == text.c_str() || *end != '\0') {
    return std::nullopt;
  }
  // On overflow strtod returns HUGE_VAL and sets ERANGE. On underflow it also
  // sets ERANGE but returns a finite value, which is kept.
  if (errno == ERANGE && std::isinf(value)) {
    return std::nullopt;
  }
  return value;
}

}  // namespace home_ems
