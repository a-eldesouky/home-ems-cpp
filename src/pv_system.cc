#include "home_ems/pv_system.h"

#include "home_ems/units.h"
#include "validation.h"

namespace home_ems {
namespace {

Voltage CheckedVoltage(Voltage voltage) {
  internal::RequirePositive(voltage.value, "PV voltage");
  return voltage;
}

Power CheckedPower(Power power) {
  internal::RequireNonNegative(power.value, "PV power");
  return power;
}

}  // namespace

PvSystem::PvSystem(Power power, Voltage voltage)
    : power_(CheckedPower(power)),
      voltage_(CheckedVoltage(voltage)),
      current_(power_.value / voltage_.value) {}

void PvSystem::SetPower(Power power) {
  power_ = CheckedPower(power);
  current_ = Current(power_.value / voltage_.value);
}

}  // namespace home_ems
