#ifndef HOME_EMS_GRID_H_
#define HOME_EMS_GRID_H_

#include "home_ems/units.h"

namespace home_ems {

// Connection to the public grid. Keeps running totals of the energy sold to
// and bought from it.
class Grid {
 public:
  Grid(Voltage voltage, Frequency frequency);

  Grid(const Grid&) = default;
  Grid& operator=(const Grid&) = default;
  Grid(Grid&&) noexcept = default;
  Grid& operator=(Grid&&) noexcept = default;
  ~Grid() = default;

  // Adds `energy` to the sold total. Throws InvalidParameterError if negative
  // or not finite.
  void SellEnergy(Energy energy);
  // Adds `energy` to the bought total. Throws InvalidParameterError if
  // negative or not finite.
  void BuyEnergy(Energy energy);

  [[nodiscard]] Energy sold_energy() const { return sold_energy_; }
  [[nodiscard]] Energy bought_energy() const { return bought_energy_; }
  [[nodiscard]] Voltage voltage() const { return voltage_; }
  [[nodiscard]] Frequency frequency() const { return frequency_; }

 private:
  Voltage voltage_;
  Frequency frequency_;
  Energy sold_energy_;
  Energy bought_energy_;
};

}  // namespace home_ems

#endif  // HOME_EMS_GRID_H_
