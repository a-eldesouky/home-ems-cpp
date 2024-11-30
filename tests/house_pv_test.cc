#include <limits>

#include <gtest/gtest.h>

#include "home_ems/errors.h"
#include "home_ems/house.h"
#include "home_ems/pv_system.h"
#include "home_ems/units.h"

namespace home_ems {
namespace {

TEST(PvSystemTest, DerivesCurrentFromPowerAndVoltage) {
  const PvSystem pv(Power(5000.0), Voltage(250.0));
  EXPECT_DOUBLE_EQ(pv.power().value, 5000.0);
  EXPECT_DOUBLE_EQ(pv.voltage().value, 250.0);
  EXPECT_DOUBLE_EQ(pv.current().value, 20.0);
}

TEST(PvSystemTest, SetPowerUpdatesCurrent) {
  PvSystem pv(Power(5000.0), Voltage(250.0));
  pv.SetPower(Power(1000.0));
  EXPECT_DOUBLE_EQ(pv.power().value, 1000.0);
  EXPECT_DOUBLE_EQ(pv.current().value, 4.0);
}

TEST(HouseTest, DerivesCurrentFromPowerAndVoltage) {
  const House house(Power(2300.0), Voltage(230.0), Frequency(50.0));
  EXPECT_DOUBLE_EQ(house.power_consumption().value, 2300.0);
  EXPECT_DOUBLE_EQ(house.voltage().value, 230.0);
  EXPECT_DOUBLE_EQ(house.frequency().value, 50.0);
  EXPECT_DOUBLE_EQ(house.current().value, 10.0);
}

TEST(HouseTest, SetPowerConsumptionUpdatesCurrent) {
  House house(Power(2300.0), Voltage(230.0), Frequency(50.0));
  house.SetPowerConsumption(Power(460.0));
  EXPECT_DOUBLE_EQ(house.power_consumption().value, 460.0);
  EXPECT_DOUBLE_EQ(house.current().value, 2.0);
}

constexpr double kNan = std::numeric_limits<double>::quiet_NaN();
constexpr double kInf = std::numeric_limits<double>::infinity();

TEST(PvSystemTest, RejectsInvalidVoltage) {
  EXPECT_THROW(PvSystem(Power(100.0), Voltage(0.0)), InvalidParameterError);
  EXPECT_THROW(PvSystem(Power(100.0), Voltage(-220.0)), InvalidParameterError);
  EXPECT_THROW(PvSystem(Power(100.0), Voltage(kNan)), InvalidParameterError);
}

TEST(PvSystemTest, RejectsInvalidPower) {
  EXPECT_THROW(PvSystem(Power(-1.0), Voltage(220.0)), InvalidParameterError);
  EXPECT_THROW(PvSystem(Power(kNan), Voltage(220.0)), InvalidParameterError);
  EXPECT_THROW(PvSystem(Power(kInf), Voltage(220.0)), InvalidParameterError);
}

TEST(PvSystemTest, RejectedSetPowerLeavesStateUnchanged) {
  PvSystem pv(Power(1000.0), Voltage(250.0));
  EXPECT_THROW(pv.SetPower(Power(kNan)), InvalidParameterError);
  EXPECT_DOUBLE_EQ(pv.power().value, 1000.0);
  EXPECT_DOUBLE_EQ(pv.current().value, 4.0);
}

TEST(HouseTest, RejectsInvalidVoltageAndFrequency) {
  EXPECT_THROW(House(Power(100.0), Voltage(0.0), Frequency(50.0)),
               InvalidParameterError);
  EXPECT_THROW(House(Power(100.0), Voltage(kInf), Frequency(50.0)),
               InvalidParameterError);
  EXPECT_THROW(House(Power(100.0), Voltage(230.0), Frequency(0.0)),
               InvalidParameterError);
  EXPECT_THROW(House(Power(100.0), Voltage(230.0), Frequency(kNan)),
               InvalidParameterError);
}

TEST(HouseTest, RejectsInvalidPower) {
  EXPECT_THROW(House(Power(-5.0), Voltage(230.0), Frequency(50.0)),
               InvalidParameterError);
  EXPECT_THROW(House(Power(kNan), Voltage(230.0), Frequency(50.0)),
               InvalidParameterError);
}

TEST(HouseTest, RejectedSetPowerLeavesStateUnchanged) {
  House house(Power(460.0), Voltage(230.0), Frequency(50.0));
  EXPECT_THROW(house.SetPowerConsumption(Power(-1.0)), InvalidParameterError);
  EXPECT_DOUBLE_EQ(house.power_consumption().value, 460.0);
  EXPECT_DOUBLE_EQ(house.current().value, 2.0);
}

}  // namespace
}  // namespace home_ems
