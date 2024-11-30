#include "home_ems/units.h"

#include <sstream>

#include <gtest/gtest.h>

namespace home_ems {
namespace {

TEST(QuantityTest, ArithmeticKeepsType) {
  const Power a(1500.0);
  const Power b(500.0);
  EXPECT_DOUBLE_EQ((a + b).value, 2000.0);
  EXPECT_DOUBLE_EQ((a - b).value, 1000.0);

  Power c(100.0);
  c += Power(50.0);
  EXPECT_DOUBLE_EQ(c.value, 150.0);
  c -= Power(200.0);
  EXPECT_DOUBLE_EQ(c.value, -50.0);
}

TEST(QuantityTest, Comparisons) {
  const Voltage low(10.0);
  const Voltage high(20.0);
  EXPECT_TRUE(low < high);
  EXPECT_TRUE(high > low);
  EXPECT_TRUE(low <= Voltage(10.0));
  EXPECT_TRUE(high >= Voltage(20.0));
  EXPECT_FALSE(low > high);
}

TEST(QuantityTest, DefaultIsZero) {
  EXPECT_DOUBLE_EQ(Current().value, 0.0);
  EXPECT_DOUBLE_EQ(Frequency().value, 0.0);
  EXPECT_DOUBLE_EQ(Temperature().value, 0.0);
}

TEST(QuantityTest, StreamsValue) {
  std::ostringstream out;
  out << Power(42.5);
  EXPECT_EQ(out.str(), "42.5");
}

TEST(PowerTest, SurplusAndRemainingAreStrictlyPositive) {
  EXPECT_TRUE(Power(0.1).IsSurplus());
  EXPECT_FALSE(Power(0.0).IsSurplus());
  EXPECT_FALSE(Power(-0.1).IsSurplus());
  EXPECT_TRUE(Power(1.0).IsRemaining());
  EXPECT_FALSE(Power(0.0).IsRemaining());
}

TEST(SystemTypeTest, ModuleCount) {
  EXPECT_EQ(ModuleCount(SystemType::kBasic), 2);
  EXPECT_EQ(ModuleCount(SystemType::kStandard), 3);
  EXPECT_EQ(ModuleCount(SystemType::kPro), 5);
}

TEST(EnergyTest, PowerTimesHoursIsEnergy) {
  EXPECT_DOUBLE_EQ((Power(2000.0) * Hours(0.25)).value, 500.0);
  EXPECT_DOUBLE_EQ((Hours(2.0) * Power(300.0)).value, 600.0);
  EXPECT_DOUBLE_EQ((Energy(500.0) / Hours(0.25)).value, 2000.0);
}

}  // namespace
}  // namespace home_ems
