#include "home_ems/inverter.h"

#include <limits>
#include <vector>

#include <gtest/gtest.h>

#include "home_ems/battery_module.h"
#include "home_ems/errors.h"
#include "home_ems/units.h"
#include "test_helpers.h"

namespace home_ems {
namespace {

using testing::MakeGrid;
using testing::MakeModule;

constexpr double kMaxPowerW = 4000.0;

TEST(InverterTest, ReportsMaxPowerAndBatteryVoltage) {
  const Inverter inverter{Power(kMaxPowerW)};
  const std::vector<BatteryModule> modules = {MakeModule()};
  EXPECT_DOUBLE_EQ(inverter.max_power().value, kMaxPowerW);
  EXPECT_DOUBLE_EQ(inverter.power().value, 0.0);
  EXPECT_DOUBLE_EQ(inverter.BatteryVoltage(modules).value, 48.0);
}

TEST(InverterTest, SetPowerToMax) {
  Inverter inverter{Power(kMaxPowerW)};
  EXPECT_NO_THROW(inverter.SetPower(Power(kMaxPowerW)));
  EXPECT_DOUBLE_EQ(inverter.power().value, kMaxPowerW);
}

TEST(InverterTest, SetPowerToNegativeMax) {
  Inverter inverter{Power(kMaxPowerW)};
  EXPECT_NO_THROW(inverter.SetPower(Power(-kMaxPowerW)));
  EXPECT_DOUBLE_EQ(inverter.power().value, -kMaxPowerW);
}

TEST(InverterTest, RejectsPowerAboveMax) {
  Inverter inverter{Power(kMaxPowerW)};
  EXPECT_THROW(inverter.SetPower(Power(kMaxPowerW + 1000.0)), OverloadError);
  EXPECT_DOUBLE_EQ(inverter.power().value, 0.0);
}

TEST(InverterTest, RejectsDischargeAboveMax) {
  Inverter inverter{Power(kMaxPowerW)};
  EXPECT_THROW(inverter.SetPower(Power(-kMaxPowerW - 0.5)), OverloadError);
}

TEST(InverterTest, RejectsInconsistentModuleVoltages) {
  const Inverter inverter{Power(kMaxPowerW)};
  const std::vector<BatteryModule> modules = {MakeModule(48.0),
                                              MakeModule(50.0)};
  EXPECT_THROW((void)inverter.BatteryVoltage(modules), HardwareFailureError);
}

TEST(InverterTest, BatteryVoltageNeedsModules) {
  const Inverter inverter{Power(kMaxPowerW)};
  EXPECT_THROW((void)inverter.BatteryVoltage({}), HardwareFailureError);
}

TEST(InverterTest, BatteryCurrentSumsModules) {
  const Inverter inverter{Power(kMaxPowerW)};
  const std::vector<BatteryModule> modules(3, MakeModule(50.0));
  // Each module: 2000 W / 50 V = 40 A.
  EXPECT_DOUBLE_EQ(inverter.BatteryCurrent(modules).value, 120.0);
}

TEST(InverterTest, BatteryCurrentNeedsModules) {
  const Inverter inverter{Power(kMaxPowerW)};
  EXPECT_THROW((void)inverter.BatteryCurrent({}), HardwareFailureError);
}

TEST(InverterTest, RejectsNonFiniteRating) {
  EXPECT_THROW(Inverter(Power(std::numeric_limits<double>::quiet_NaN())),
               InvalidParameterError);
  EXPECT_THROW(Inverter(Power(std::numeric_limits<double>::infinity())),
               InvalidParameterError);
}

TEST(InverterTest, RejectsNonFinitePower) {
  Inverter inverter{Power(kMaxPowerW)};
  inverter.SetPower(Power(100.0));
  EXPECT_THROW(
      inverter.SetPower(Power(std::numeric_limits<double>::quiet_NaN())),
      InvalidParameterError);
  EXPECT_DOUBLE_EQ(inverter.power().value, 100.0);
}

TEST(InverterTest, ReadsGridValues) {
  const Inverter inverter{Power(kMaxPowerW)};
  const auto grid = MakeGrid();
  EXPECT_DOUBLE_EQ(inverter.GridVoltage(grid).value, 230.0);
  EXPECT_DOUBLE_EQ(inverter.GridFrequency(grid).value, 50.0);
}

TEST(InverterTest, BatteryVoltageAcceptsSmallDifferences) {
  const Inverter inverter{Power(kMaxPowerW)};
  // Default tolerance: max(0.5 V, 1% of 48 V = 0.48 V) = 0.5 V.
  const std::vector<BatteryModule> modules = {
      MakeModule(48.0), MakeModule(48.4), MakeModule(47.6)};
  EXPECT_DOUBLE_EQ(inverter.BatteryVoltage(modules).value, 48.0);
}

TEST(InverterTest, BatteryVoltageReturnsMean) {
  const Inverter inverter{Power(kMaxPowerW)};
  const std::vector<BatteryModule> modules = {MakeModule(48.0),
                                              MakeModule(48.3)};
  EXPECT_DOUBLE_EQ(inverter.BatteryVoltage(modules).value, 48.15);
}

TEST(InverterTest, BatteryVoltageRejectsJustAboveTolerance) {
  const Inverter inverter{Power(kMaxPowerW)};
  const std::vector<BatteryModule> modules = {MakeModule(48.0),
                                              MakeModule(48.51)};
  EXPECT_THROW((void)inverter.BatteryVoltage(modules), HardwareFailureError);
}

TEST(InverterTest, BatteryVoltageUsesRelativeToleranceWhenLarger) {
  const Inverter inverter{Power(kMaxPowerW)};
  // 1% of 400 V = 4 V is larger than 0.5 V.
  const std::vector<BatteryModule> modules = {MakeModule(400.0),
                                              MakeModule(403.0)};
  EXPECT_DOUBLE_EQ(inverter.BatteryVoltage(modules).value, 401.5);
}

TEST(InverterTest, BatteryVoltageWithZeroToleranceIsExact) {
  const Inverter inverter{Power(kMaxPowerW)};
  const VoltageTolerance exact{Voltage(0.0), 0.0};
  EXPECT_THROW((void)inverter.BatteryVoltage(
                   {MakeModule(48.0), MakeModule(48.01)}, exact),
               HardwareFailureError);
  EXPECT_DOUBLE_EQ(
      inverter.BatteryVoltage({MakeModule(48.0), MakeModule(48.0)}, exact)
          .value,
      48.0);
}

TEST(InverterTest, BatteryVoltageRejectsInvalidTolerance) {
  const Inverter inverter{Power(kMaxPowerW)};
  const std::vector<BatteryModule> modules = {MakeModule()};
  EXPECT_THROW((void)inverter.BatteryVoltage(modules, {Voltage(-1.0), 0.01}),
               InvalidParameterError);
  EXPECT_THROW(
      (void)inverter.BatteryVoltage(
          modules, {Voltage(0.5), std::numeric_limits<double>::quiet_NaN()}),
      InvalidParameterError);
}

}  // namespace
}  // namespace home_ems
