#include "home_ems/battery_storage.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "home_ems/battery_module.h"
#include "home_ems/errors.h"
#include "home_ems/inverter.h"
#include "home_ems/units.h"
#include "validation.h"

namespace home_ems {
namespace {

Power SumOfRatings(const std::vector<BatteryModule>& modules) {
  Power total(kZero);
  for (const BatteryModule& module : modules) {
    total += module.max_power();
  }
  return total;
}

void RequireEfficiency(double value, const char* what) {
  if (!(value > 0.0 && value <= 1.0)) {
    throw InvalidParameterError(std::string(what) + " must be in (0, 1].");
  }
}

void Validate(const StateOfChargeModel& model) {
  internal::RequirePositive(model.module_capacity.value, "Module capacity");
  if (!(model.min_soc >= 0.0 && model.min_soc < model.max_soc &&
        model.max_soc <= 1.0)) {
    throw InvalidParameterError(
        "State of charge limits must satisfy 0 <= min < max <= 1.");
  }
  if (!(model.initial_soc >= model.min_soc &&
        model.initial_soc <= model.max_soc)) {
    throw InvalidParameterError(
        "Initial state of charge must be between the min and max limits.");
  }
  RequireEfficiency(model.charge_efficiency, "Charge efficiency");
  RequireEfficiency(model.discharge_efficiency, "Discharge efficiency");
}

}  // namespace

BatteryStorage::BatteryStorage(Inverter inverter,
                               std::vector<BatteryModule> modules,
                               std::optional<StateOfChargeModel> soc_model)
    : power_command_(kZero),
      inverter_(inverter),
      modules_(std::move(modules)),
      module_max_power_(SumOfRatings(modules_)),
      soc_model_(soc_model),
      capacity_(kZero),
      min_energy_(kZero),
      max_energy_(kZero),
      stored_energy_(kZero) {
  if (modules_.empty()) {
    throw HardwareFailureError(
        "Battery storage must have at least one battery module.");
  }
  if (soc_model_) {
    Validate(*soc_model_);
    capacity_ = Energy(soc_model_->module_capacity.value *
                       static_cast<double>(modules_.size()));
    min_energy_ = Energy(soc_model_->min_soc * capacity_.value);
    max_energy_ = Energy(soc_model_->max_soc * capacity_.value);
    stored_energy_ = Energy(soc_model_->initial_soc * capacity_.value);
  }
}

Power BatteryStorage::max_power() const {
  return Power(std::min(inverter_.max_power().value, module_max_power_.value));
}

Power BatteryStorage::MaxChargePower(Hours step) const {
  internal::RequirePositive(step.value, "Step length");
  const Power rating = max_power();
  if (!soc_model_) {
    return rating;
  }
  const double room_wh =
      std::max(kZero, max_energy_.value - stored_energy_.value);
  const double limit_w = room_wh / (step.value * soc_model_->charge_efficiency);
  return Power(std::min(rating.value, limit_w));
}

Power BatteryStorage::MaxDischargePower(Hours step) const {
  internal::RequirePositive(step.value, "Step length");
  const Power rating = max_power();
  if (!soc_model_) {
    return rating;
  }
  const double available_wh =
      std::max(kZero, stored_energy_.value - min_energy_.value);
  const double limit_w =
      available_wh * soc_model_->discharge_efficiency / step.value;
  return Power(std::min(rating.value, limit_w));
}

void BatteryStorage::SetPowerCommand(PowerCommand command, Power power,
                                     Hours step) {
  internal::RequirePositive(step.value, "Step length");
  internal::RequireFinite(power.value, "Battery power");
  const Power new_command =
      command == PowerCommand::kCharge ? power : Power(-power.value);
  if (std::abs(new_command.value) > module_max_power_.value) {
    throw OverloadError("Power exceeds battery modules' max capacity.");
  }
  if (soc_model_) {
    if (new_command.value > MaxChargePower(step).value) {
      throw OverloadError("Charge power exceeds the room left in the battery.");
    }
    if (-new_command.value > MaxDischargePower(step).value) {
      throw OverloadError(
          "Discharge power exceeds the energy left in the battery.");
    }
  }
  // Inverter::SetPower checks its own rating and does not change its set point
  // on a throw, so nothing is stored before it succeeds.
  inverter_.SetPower(new_command);
  power_command_ = new_command;

  if (soc_model_) {
    const double drawn_wh = new_command.value * step.value;
    const double change_wh = drawn_wh >= kZero
                                 ? drawn_wh * soc_model_->charge_efficiency
                                 : drawn_wh / soc_model_->discharge_efficiency;
    // The limits above keep the result in range; the clamp only removes
    // rounding error.
    stored_energy_ = Energy(std::clamp(stored_energy_.value + change_wh,
                                       min_energy_.value, max_energy_.value));
  }
}

std::optional<Energy> BatteryStorage::capacity() const {
  if (!soc_model_) {
    return std::nullopt;
  }
  return capacity_;
}

std::optional<Energy> BatteryStorage::stored_energy() const {
  if (!soc_model_) {
    return std::nullopt;
  }
  return stored_energy_;
}

std::optional<double> BatteryStorage::state_of_charge() const {
  if (!soc_model_) {
    return std::nullopt;
  }
  return stored_energy_.value / capacity_.value;
}

}  // namespace home_ems
