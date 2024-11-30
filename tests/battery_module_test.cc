#include "home_ems/battery_module.h"

#include <limits>

#include <gtest/gtest.h>

#include "home_ems/errors.h"
#include "home_ems/units.h"

namespace home_ems {
namespace {

TEST(BatteryModuleTest, StoresValues) {
  const BatteryModule module(Temperature(25.0), Voltage(48.0), Power(2000.0));
  EXPECT_DOUBLE_EQ(module.temperature().value, 25.0);
  EXPECT_DOUBLE_EQ(module.voltage().value, 48.0);
  EXPECT_DOUBLE_EQ(module.max_power().value, 2000.0);
}

TEST(BatteryModuleTest, AcceptsTemperatureLimits) {
  EXPECT_NO_THROW(BatteryModule(Temperature(BatteryModule::kMaxTemperatureC),
                                Voltage(48.0), Power(2000.0)));
  EXPECT_NO_THROW(BatteryModule(Temperature(BatteryModule::kMinTemperatureC),
                                Voltage(48.0), Power(2000.0)));
}

TEST(BatteryModuleTest, RejectsNegativeVoltage) {
  EXPECT_THROW(BatteryModule(Temperature(25.0), Voltage(-48.0), Power(2000.0)),
               InvalidParameterError);
}

TEST(BatteryModuleTest, RejectsZeroVoltage) {
  EXPECT_THROW(BatteryModule(Temperature(25.0), Voltage(0.0), Power(2000.0)),
               InvalidParameterError);
}

TEST(BatteryModuleTest, RejectsNonPositiveMaxPower) {
  EXPECT_THROW(BatteryModule(Temperature(25.0), Voltage(48.0), Power(0.0)),
               InvalidParameterError);
  EXPECT_THROW(BatteryModule(Temperature(25.0), Voltage(48.0), Power(-1.0)),
               InvalidParameterError);
}

TEST(BatteryModuleTest, RejectsTemperatureBelowRange) {
  EXPECT_THROW(BatteryModule(Temperature(-30.0), Voltage(48.0), Power(2000.0)),
               InvalidParameterError);
}

TEST(BatteryModuleTest, RejectsTemperatureAboveRange) {
  EXPECT_THROW(BatteryModule(Temperature(60.5), Voltage(48.0), Power(2000.0)),
               InvalidParameterError);
}

TEST(BatteryModuleTest, RejectsNonFiniteValues) {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const double inf = std::numeric_limits<double>::infinity();
  EXPECT_THROW(BatteryModule(Temperature(25.0), Voltage(nan), Power(2000.0)),
               InvalidParameterError);
  EXPECT_THROW(BatteryModule(Temperature(nan), Voltage(48.0), Power(2000.0)),
               InvalidParameterError);
  EXPECT_THROW(BatteryModule(Temperature(25.0), Voltage(48.0), Power(nan)),
               InvalidParameterError);
  EXPECT_THROW(BatteryModule(Temperature(25.0), Voltage(48.0), Power(inf)),
               InvalidParameterError);
  EXPECT_THROW(BatteryModule(Temperature(25.0), Voltage(inf), Power(2000.0)),
               InvalidParameterError);
}

}  // namespace
}  // namespace home_ems
