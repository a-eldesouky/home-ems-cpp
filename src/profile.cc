#include "home_ems/profile.h"

#include <cmath>
#include <cstddef>
#include <istream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "home_ems/errors.h"
#include "home_ems/parse_number.h"
#include "home_ems/units.h"

namespace home_ems {
namespace {

constexpr std::size_t kFieldCount = 3;

[[noreturn]] void Fail(int line_number, const std::string& message) {
  throw InvalidParameterError("line " + std::to_string(line_number) + ": " +
                              message);
}

void StripCarriageReturn(std::string& line) {
  if (!line.empty() && line.back() == '\r') {
    line.pop_back();
  }
}

std::vector<std::string> SplitFields(const std::string& line) {
  std::vector<std::string> fields;
  std::string::size_type start = 0;
  while (true) {
    const std::string::size_type comma = line.find(',', start);
    if (comma == std::string::npos) {
      fields.push_back(line.substr(start));
      return fields;
    }
    fields.push_back(line.substr(start, comma - start));
    start = comma + 1;
  }
}

Power ParsePower(const std::string& field, std::string_view name,
                 int line_number) {
  if (field.empty()) {
    Fail(line_number, std::string(name) + " is empty");
  }
  const std::optional<double> parsed = ParseNumber(field);
  if (!parsed) {
    Fail(line_number, std::string(name) + " is not a number: '" + field + "'");
  }
  const double value = *parsed;
  if (!std::isfinite(value)) {
    Fail(line_number, std::string(name) + " is not finite");
  }
  if (value < kZero) {
    Fail(line_number, std::string(name) + " must be >= 0");
  }
  return Power(value);
}

ProfileSample ParseRow(const std::string& line, int line_number) {
  std::vector<std::string> fields = SplitFields(line);
  if (fields.size() != kFieldCount) {
    Fail(line_number, "expected " + std::to_string(kFieldCount) +
                          " fields, got " + std::to_string(fields.size()));
  }
  if (fields[0].empty()) {
    Fail(line_number, "time is empty");
  }
  const Power pv = ParsePower(fields[1], "pv_w", line_number);
  const Power load = ParsePower(fields[2], "load_w", line_number);
  return {std::move(fields[0]), pv, load, line_number};
}

constexpr int kMinutesPerHour = 60;
constexpr int kMinutesPerDay = 24 * kMinutesPerHour;

bool IsDigit(char c) { return c >= '0' && c <= '9'; }

int TwoDigits(char tens, char ones) { return (tens - '0') * 10 + (ones - '0'); }

}  // namespace

std::optional<int> ParseClockMinutes(const std::string& label) {
  if (label.size() != 5 || label[2] != ':' || !IsDigit(label[0]) ||
      !IsDigit(label[1]) || !IsDigit(label[3]) || !IsDigit(label[4])) {
    return std::nullopt;
  }
  const int hours = TwoDigits(label[0], label[1]);
  const int minutes = TwoDigits(label[3], label[4]);
  if (hours > 23 || minutes > 59) {
    return std::nullopt;
  }
  return hours * kMinutesPerHour + minutes;
}

void CheckTimeSpacing(const std::vector<ProfileSample>& samples,
                      int step_minutes) {
  if (step_minutes < 1 || step_minutes > kMinutesPerDay) {
    throw InvalidParameterError("Step length must be 1 to 1440 minutes.");
  }
  if (samples.empty()) {
    return;
  }
  const std::optional<int> first = ParseClockMinutes(samples.front().time);
  if (!first) {
    return;  // Free-form labels are not checked.
  }
  int previous = *first;
  for (std::size_t i = 1; i < samples.size(); ++i) {
    const ProfileSample& sample = samples[i];
    const std::optional<int> now = ParseClockMinutes(sample.time);
    if (!now) {
      Fail(sample.line, "time '" + sample.time + "' is not HH:MM");
    }
    const int gap = (*now - previous + kMinutesPerDay) % kMinutesPerDay;
    if (gap != step_minutes % kMinutesPerDay) {
      Fail(sample.line, "time '" + sample.time + "' is not " +
                            std::to_string(step_minutes) + " minutes after '" +
                            samples[i - 1].time + "'");
    }
    previous = *now;
  }
}

std::vector<ProfileSample> ReadProfile(std::istream& in) {
  std::string line;
  int line_number = 1;
  if (!std::getline(in, line)) {
    Fail(line_number, "missing header '" + std::string(kProfileHeader) + "'");
  }
  StripCarriageReturn(line);
  if (line != kProfileHeader) {
    Fail(line_number, "expected header '" + std::string(kProfileHeader) +
                          "', got '" + line + "'");
  }

  std::vector<ProfileSample> samples;
  while (std::getline(in, line)) {
    ++line_number;
    StripCarriageReturn(line);
    if (line.empty()) {
      continue;
    }
    samples.push_back(ParseRow(line, line_number));
  }
  return samples;
}

}  // namespace home_ems
