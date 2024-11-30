#include "home_ems/energy_controller.h"

#include <cmath>
#include <limits>
#include <vector>

#include <gtest/gtest.h>

#include "home_ems/battery_module.h"
#include "home_ems/battery_storage.h"
#include "home_ems/grid.h"
#include "home_ems/inverter.h"
#include "home_ems/units.h"
#include "test_helpers.h"

namespace home_ems {
namespace {

using testing::MakeGrid;
using testing::MakeHouse;
using testing::MakePv;
using testing::MakeStorage;

TEST(ControlStepTest, SurplusChargesBattery) {
  BatteryStorage storage = MakeStorage();
  Grid grid = MakeGrid();

  const ControlResult r = ControlStep(MakePv(5000.0), MakeHouse(3000.0),
                                      storage, grid, testing::kHour);

  EXPECT_TRUE(r.error.empty());
  EXPECT_DOUBLE_EQ(r.net.value, 2000.0);
  EXPECT_DOUBLE_EQ(r.battery_command.value, 2000.0);
  EXPECT_DOUBLE_EQ(storage.power_command().value, 2000.0);
  EXPECT_DOUBLE_EQ(storage.inverter().power().value, 2000.0);
  EXPECT_DOUBLE_EQ(r.grid_export.value, 0.0);
  EXPECT_DOUBLE_EQ(grid.sold_energy().value, 0.0);
}

TEST(ControlStepTest, ShortageDischargesBattery) {
  BatteryStorage storage = MakeStorage();
  Grid grid = MakeGrid();

  const ControlResult r = ControlStep(MakePv(2000.0), MakeHouse(3000.0),
                                      storage, grid, testing::kHour);

  EXPECT_TRUE(r.error.empty());
  EXPECT_DOUBLE_EQ(r.net.value, -1000.0);
  EXPECT_DOUBLE_EQ(r.battery_command.value, -1000.0);
  EXPECT_DOUBLE_EQ(storage.inverter().power().value, -1000.0);
  EXPECT_DOUBLE_EQ(r.grid_import.value, 0.0);
  EXPECT_DOUBLE_EQ(grid.bought_energy().value, 0.0);
}

TEST(ControlStepTest, ZeroNetPowerIsZeroDischarge) {
  BatteryStorage storage = MakeStorage();
  Grid grid = MakeGrid();

  const ControlResult r = ControlStep(MakePv(3000.0), MakeHouse(3000.0),
                                      storage, grid, testing::kHour);

  EXPECT_TRUE(r.error.empty());
  EXPECT_DOUBLE_EQ(r.battery_command.value, 0.0);
  // Zero net power takes the shortage path: -(-0.0) gives +0.0.
  EXPECT_FALSE(std::signbit(r.battery_command.value));
  EXPECT_DOUBLE_EQ(storage.inverter().power().value, 0.0);
  EXPECT_DOUBLE_EQ(grid.sold_energy().value, 0.0);
  EXPECT_DOUBLE_EQ(grid.bought_energy().value, 0.0);
}

TEST(ControlStepTest, SurplusAboveInverterLimitIsSold) {
  BatteryStorage storage = MakeStorage(4000.0);
  Grid grid = MakeGrid();

  const ControlResult r = ControlStep(MakePv(6500.0), MakeHouse(500.0), storage,
                                      grid, testing::kHour);

  EXPECT_DOUBLE_EQ(r.battery_command.value, 4000.0);
  EXPECT_DOUBLE_EQ(r.grid_export.value, 2000.0);
  EXPECT_DOUBLE_EQ(grid.sold_energy().value, 2000.0);
}

TEST(ControlStepTest, ShortageAboveInverterLimitIsBought) {
  BatteryStorage storage = MakeStorage(4000.0);
  Grid grid = MakeGrid();

  const ControlResult r = ControlStep(MakePv(0.0), MakeHouse(4500.0), storage,
                                      grid, testing::kHour);

  EXPECT_DOUBLE_EQ(r.battery_command.value, -4000.0);
  EXPECT_DOUBLE_EQ(r.grid_import.value, 500.0);
  EXPECT_DOUBLE_EQ(grid.bought_energy().value, 500.0);
}

TEST(ControlStepTest, SurplusEqualToLimitSellsNothing) {
  BatteryStorage storage = MakeStorage(4000.0);
  Grid grid = MakeGrid();

  const ControlResult r = ControlStep(MakePv(4500.0), MakeHouse(500.0), storage,
                                      grid, testing::kHour);

  EXPECT_DOUBLE_EQ(r.battery_command.value, 4000.0);
  EXPECT_DOUBLE_EQ(r.grid_export.value, 0.0);
}

TEST(ControlStepTest, ModuleRatingLimitsCharge) {
  // One 2000 W module behind a 4000 W inverter: the modules are the limit.
  BatteryStorage storage = MakeStorage(4000.0, 1);
  Grid grid = MakeGrid();

  const ControlResult r = ControlStep(MakePv(3500.0), MakeHouse(500.0), storage,
                                      grid, testing::kHour);

  EXPECT_TRUE(r.error.empty());
  EXPECT_DOUBLE_EQ(r.battery_command.value, 2000.0);
  EXPECT_DOUBLE_EQ(r.grid_export.value, 1000.0);
}

TEST(ControlStepTest, ModuleRatingLimitsDischarge) {
  BatteryStorage storage = MakeStorage(4000.0, 1);
  Grid grid = MakeGrid();

  const ControlResult r = ControlStep(MakePv(0.0), MakeHouse(3000.0), storage,
                                      grid, testing::kHour);

  EXPECT_TRUE(r.error.empty());
  EXPECT_DOUBLE_EQ(r.battery_command.value, -2000.0);
  EXPECT_DOUBLE_EQ(r.grid_import.value, 1000.0);
}

TEST(ControlStepTest, GridTotalsAccumulateOverSteps) {
  BatteryStorage storage = MakeStorage(1000.0);
  Grid grid = MakeGrid();

  (void)ControlStep(MakePv(3000.0), MakeHouse(1000.0), storage, grid,
                    testing::kHour);
  (void)ControlStep(MakePv(2500.0), MakeHouse(1000.0), storage, grid,
                    testing::kHour);

  EXPECT_DOUBLE_EQ(grid.sold_energy().value, 1000.0 + 500.0);
}

TEST(ControlStepTest, ZeroInverterLimitAbortsSurplusStep) {
  BatteryStorage storage = MakeStorage(0.0);
  Grid grid = MakeGrid();

  const ControlResult r = ControlStep(MakePv(5000.0), MakeHouse(3000.0),
                                      storage, grid, testing::kHour);

  EXPECT_EQ(r.error, "Invalid storage max charge power.");
  EXPECT_DOUBLE_EQ(r.battery_command.value, 0.0);
  EXPECT_DOUBLE_EQ(grid.sold_energy().value, 0.0);
}

TEST(ControlStepTest, NegativeInverterLimitAbortsShortageStep) {
  BatteryStorage storage = MakeStorage(-100.0);
  Grid grid = MakeGrid();

  const ControlResult r = ControlStep(MakePv(1000.0), MakeHouse(3000.0),
                                      storage, grid, testing::kHour);

  EXPECT_EQ(r.error, "Invalid storage max discharge power.");
  EXPECT_DOUBLE_EQ(grid.bought_energy().value, 0.0);
}

TEST(ControlStepTest, InvalidStepAbortsStep) {
  for (const double hours : {0.0, -1.0, std::numeric_limits<double>::infinity(),
                             std::numeric_limits<double>::quiet_NaN()}) {
    BatteryStorage storage = MakeStorage();
    Grid grid = MakeGrid();
    const ControlResult r = ControlStep(MakePv(5000.0), MakeHouse(0.0), storage,
                                        grid, Hours(hours));
    EXPECT_EQ(r.error, "Step length must be a finite number > 0.");
    EXPECT_DOUBLE_EQ(r.battery_command.value, 0.0);
    EXPECT_DOUBLE_EQ(grid.sold_energy().value, 0.0);
  }
}

TEST(ControlStepTest, FailedStepDoesNotRepeatPreviousCommand) {
  BatteryStorage storage = MakeStorage();
  Grid grid = MakeGrid();
  const ControlResult ok = ControlStep(MakePv(3000.0), MakeHouse(1000.0),
                                       storage, grid, testing::kHour);
  ASSERT_TRUE(ok.error.empty());
  ASSERT_DOUBLE_EQ(ok.battery_command.value, 2000.0);

  const ControlResult failed =
      ControlStep(MakePv(0.0), MakeHouse(500.0), storage, grid, Hours(0.0));

  EXPECT_FALSE(failed.error.empty());
  EXPECT_DOUBLE_EQ(failed.battery_command.value, 0.0);
  EXPECT_DOUBLE_EQ(failed.grid_import.value, 0.0);
  EXPECT_DOUBLE_EQ(grid.bought_energy().value, 0.0);
}

TEST(ControlStepTest, GridEnergyUsesStepLength) {
  BatteryStorage storage = MakeStorage(4000.0);
  Grid grid = MakeGrid();
  (void)ControlStep(MakePv(6000.0), MakeHouse(0.0), storage, grid, Hours(0.25));
  EXPECT_DOUBLE_EQ(grid.sold_energy().value, 500.0);
}

BatteryStorage MakeSocStorage(double module_capacity_wh, double initial_soc) {
  StateOfChargeModel model;
  model.module_capacity = Energy(module_capacity_wh);
  model.min_soc = 0.1;
  model.max_soc = 0.9;
  model.initial_soc = initial_soc;
  return {Inverter(Power(4000.0)),
          std::vector<BatteryModule>(3, testing::MakeModule()), model};
}

TEST(ControlStepTest, FullBatteryExportsEverySurplus) {
  // 3 x 1000 Wh = 3000 Wh, already at max_soc.
  BatteryStorage storage = MakeSocStorage(1000.0, 0.9);
  Grid grid = MakeGrid();
  const ControlResult r = ControlStep(MakePv(3000.0), MakeHouse(1000.0),
                                      storage, grid, testing::kHour);
  EXPECT_TRUE(r.error.empty());
  EXPECT_DOUBLE_EQ(r.battery_command.value, 0.0);
  EXPECT_DOUBLE_EQ(r.grid_export.value, 2000.0);
  EXPECT_DOUBLE_EQ(r.state_of_charge.value_or(0.0), 0.9);
}

TEST(ControlStepTest, EmptyBatteryImportsEveryShortage) {
  BatteryStorage storage = MakeSocStorage(1000.0, 0.1);
  Grid grid = MakeGrid();
  const ControlResult r = ControlStep(MakePv(0.0), MakeHouse(1500.0), storage,
                                      grid, testing::kHour);
  EXPECT_TRUE(r.error.empty());
  EXPECT_DOUBLE_EQ(r.battery_command.value, 0.0);
  EXPECT_DOUBLE_EQ(r.grid_import.value, 1500.0);
}

TEST(ControlStepTest, BatteryFillsUpThenExports) {
  // Room: 0.9 * 3000 - 0.5 * 3000 = 1200 Wh.
  BatteryStorage storage = MakeSocStorage(1000.0, 0.5);
  Grid grid = MakeGrid();
  const ControlResult first = ControlStep(MakePv(1000.0), MakeHouse(0.0),
                                          storage, grid, testing::kHour);
  EXPECT_DOUBLE_EQ(first.battery_command.value, 1000.0);
  EXPECT_DOUBLE_EQ(first.grid_export.value, 0.0);
  const ControlResult second = ControlStep(MakePv(1000.0), MakeHouse(0.0),
                                           storage, grid, testing::kHour);
  EXPECT_DOUBLE_EQ(second.battery_command.value, 200.0);
  EXPECT_DOUBLE_EQ(second.grid_export.value, 800.0);
  EXPECT_NEAR(second.state_of_charge.value_or(0.0), 0.9, 1e-12);
  const ControlResult third = ControlStep(MakePv(1000.0), MakeHouse(0.0),
                                          storage, grid, testing::kHour);
  EXPECT_DOUBLE_EQ(third.battery_command.value, 0.0);
  EXPECT_DOUBLE_EQ(third.grid_export.value, 1000.0);
}

TEST(ControlStepTest, ShortStepAllowsHigherPowerForSameEnergy) {
  // 300 Wh of room. In a 6-minute step that is up to 3000 W.
  BatteryStorage storage = MakeSocStorage(1000.0, 0.8);
  Grid grid = MakeGrid();
  const ControlResult r =
      ControlStep(MakePv(3500.0), MakeHouse(0.0), storage, grid, Hours(0.1));
  EXPECT_NEAR(r.battery_command.value, 3000.0, 1e-9);
  EXPECT_NEAR(r.grid_export.value, 500.0, 1e-9);
  EXPECT_NEAR(r.state_of_charge.value_or(0.0), 0.9, 1e-12);
}

}  // namespace
}  // namespace home_ems
