#include "home_ems/grid.h"

#include <cmath>

#include "home_ems/errors.h"
#include "home_ems/units.h"

namespace home_ems {

Grid::Grid(Voltage voltage, Frequency frequency)
    : voltage_(voltage),
      frequency_(frequency),
      sold_energy_(kZero),
      bought_energy_(kZero) {}

void Grid::SellEnergy(Energy energy) {
  if (!(std::isfinite(energy.value) && energy.value >= kZero)) {
    throw InvalidParameterError(
        "Cannot sell negative or non-finite energy to grid.");
  }
  sold_energy_ += energy;
}

void Grid::BuyEnergy(Energy energy) {
  if (!(std::isfinite(energy.value) && energy.value >= kZero)) {
    throw InvalidParameterError(
        "Cannot buy negative or non-finite energy from grid.");
  }
  bought_energy_ += energy;
}

}  // namespace home_ems
