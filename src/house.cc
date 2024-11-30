#include "home_ems/house.h"

#include "home_ems/units.h"
#include "validation.h"

namespace home_ems {
namespace {

Power CheckedPower(Power power) {
  internal::RequireNonNegative(power.value, "House power");
  return power;
}

Voltage CheckedVoltage(Voltage voltage) {
  internal::RequirePositive(voltage.value, "House voltage");
  return voltage;
}

Frequency CheckedFrequency(Frequency frequency) {
  internal::RequirePositive(frequency.value, "House frequency");
  return frequency;
}

}  // namespace

House::House(Power power, Voltage voltage, Frequency frequency)
    : power_(CheckedPower(power)),
      voltage_(CheckedVoltage(voltage)),
      frequency_(CheckedFrequency(frequency)),
      current_(power_.value / voltage_.value) {}

void House::SetPowerConsumption(Power power) {
  power_ = CheckedPower(power);
  current_ = Current(power_.value / voltage_.value);
}

}  // namespace home_ems
