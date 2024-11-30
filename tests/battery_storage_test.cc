#include "home_ems/battery_storage.h"

#include <limits>
#include <vector>

#include <gtest/gtest.h>

#include "home_ems/battery_module.h"
#include "home_ems/errors.h"
#include "home_ems/inverter.h"
#include "home_ems/units.h"
#include "test_helpers.h"

namespace home_ems {
namespace {

using testing::MakeStorage;

TEST(BatteryStorageTest, RejectsEmptyModuleList) {
  EXPECT_THROW(BatteryStorage(Inverter(Power(4000.0)), {}),
               HardwareFailureError);
}

TEST(BatteryStorageTest, KeepsModulesAndInverter) {
  const BatteryStorage storage = MakeStorage(3000.0, 3);
  EXPECT_EQ(storage.modules().size(), 3U);
  EXPECT_DOUBLE_EQ(storage.inverter().max_power().value, 3000.0);
  EXPECT_DOUBLE_EQ(storage.power_command().value, 0.0);
}

TEST(BatteryStorageTest, ChargeCommandIsPositive) {
  BatteryStorage storage = MakeStorage();
  storage.SetPowerCommand(PowerCommand::kCharge, Power(1500.0), testing::kHour);
  EXPECT_DOUBLE_EQ(storage.power_command().value, 1500.0);
  EXPECT_DOUBLE_EQ(storage.inverter().power().value, 1500.0);
}

TEST(BatteryStorageTest, DischargeCommandIsNegative) {
  BatteryStorage storage = MakeStorage();
  storage.SetPowerCommand(PowerCommand::kDischarge, Power(1500.0),
                          testing::kHour);
  EXPECT_DOUBLE_EQ(storage.power_command().value, -1500.0);
  EXPECT_DOUBLE_EQ(storage.inverter().power().value, -1500.0);
}

TEST(BatteryStorageTest, InverterOverloadLeavesStateUnchanged) {
  BatteryStorage storage = MakeStorage(1000.0);
  storage.SetPowerCommand(PowerCommand::kCharge, Power(600.0), testing::kHour);
  EXPECT_THROW(storage.SetPowerCommand(PowerCommand::kCharge, Power(1500.0),
                                       testing::kHour),
               OverloadError);
  EXPECT_DOUBLE_EQ(storage.power_command().value, 600.0);
  EXPECT_DOUBLE_EQ(storage.inverter().power().value, 600.0);
}

TEST(BatteryStorageTest, ModuleOverloadLeavesStateUnchanged) {
  // One 2000 W module behind a 4000 W inverter.
  BatteryStorage storage = MakeStorage(4000.0, 1);
  EXPECT_THROW(storage.SetPowerCommand(PowerCommand::kDischarge, Power(2500.0),
                                       testing::kHour),
               OverloadError);
  EXPECT_DOUBLE_EQ(storage.power_command().value, 0.0);
  EXPECT_DOUBLE_EQ(storage.inverter().power().value, 0.0);
  EXPECT_NO_THROW(storage.SetPowerCommand(PowerCommand::kDischarge,
                                          Power(2000.0), testing::kHour));
  EXPECT_DOUBLE_EQ(storage.power_command().value, -2000.0);
}

TEST(BatteryStorageTest, NonFiniteCommandLeavesStateUnchanged) {
  BatteryStorage storage = MakeStorage();
  const double nan = std::numeric_limits<double>::quiet_NaN();
  EXPECT_THROW(storage.SetPowerCommand(PowerCommand::kCharge, Power(nan),
                                       testing::kHour),
               InvalidParameterError);
  EXPECT_DOUBLE_EQ(storage.power_command().value, 0.0);
  EXPECT_DOUBLE_EQ(storage.inverter().power().value, 0.0);
}

TEST(BatteryStorageTest, MaxPowerIsSmallerOfInverterAndModules) {
  EXPECT_DOUBLE_EQ(MakeStorage(4000.0, 3).max_power().value, 4000.0);
  EXPECT_DOUBLE_EQ(MakeStorage(4000.0, 1).max_power().value, 2000.0);
  EXPECT_DOUBLE_EQ(MakeStorage(4000.0, 2).max_power().value, 4000.0);
  EXPECT_DOUBLE_EQ(MakeStorage(4000.0, 5).module_max_power().value, 10000.0);
}

using testing::kHour;

StateOfChargeModel Model(double module_capacity_wh = 1000.0) {
  StateOfChargeModel model;
  model.module_capacity = Energy(module_capacity_wh);
  model.min_soc = 0.1;
  model.max_soc = 0.9;
  model.initial_soc = 0.5;
  return model;
}

BatteryStorage MakeWithModel(const StateOfChargeModel& model,
                             double inverter_max_w = 4000.0) {
  return {Inverter(Power(inverter_max_w)),
          std::vector<BatteryModule>(3, testing::MakeModule()), model};
}

TEST(BatteryStorageTest, WithoutModelHasNoStateOfCharge) {
  const BatteryStorage storage = MakeStorage();
  EXPECT_FALSE(storage.capacity().has_value());
  EXPECT_FALSE(storage.stored_energy().has_value());
  EXPECT_FALSE(storage.state_of_charge().has_value());
  EXPECT_DOUBLE_EQ(storage.MaxChargePower(kHour).value, 4000.0);
  EXPECT_DOUBLE_EQ(storage.MaxDischargePower(kHour).value, 4000.0);
}

TEST(BatteryStorageTest, ModelSetsCapacityAndInitialCharge) {
  const BatteryStorage storage = MakeWithModel(Model());
  EXPECT_DOUBLE_EQ(storage.capacity().value_or(Energy(0.0)).value, 3000.0);
  EXPECT_DOUBLE_EQ(storage.stored_energy().value_or(Energy(0.0)).value, 1500.0);
  EXPECT_DOUBLE_EQ(storage.state_of_charge().value_or(0.0), 0.5);
}

TEST(BatteryStorageTest, ChargeLimitFollowsRoomAndStep) {
  const BatteryStorage storage = MakeWithModel(Model());
  // Room: 2700 - 1500 = 1200 Wh.
  EXPECT_DOUBLE_EQ(storage.MaxChargePower(kHour).value, 1200.0);
  EXPECT_DOUBLE_EQ(storage.MaxChargePower(Hours(0.5)).value, 2400.0);
  // A short step is capped by the 4000 W rating.
  EXPECT_DOUBLE_EQ(storage.MaxChargePower(Hours(0.1)).value, 4000.0);
}

TEST(BatteryStorageTest, DischargeLimitFollowsEnergyAndStep) {
  const BatteryStorage storage = MakeWithModel(Model());
  // Available: 1500 - 300 = 1200 Wh.
  EXPECT_DOUBLE_EQ(storage.MaxDischargePower(kHour).value, 1200.0);
  EXPECT_DOUBLE_EQ(storage.MaxDischargePower(Hours(2.0)).value, 600.0);
}

TEST(BatteryStorageTest, EfficiencyAffectsLimitsAndStoredEnergy) {
  StateOfChargeModel model = Model();
  model.charge_efficiency = 0.8;
  model.discharge_efficiency = 0.5;
  BatteryStorage storage = MakeWithModel(model);
  // 1200 Wh of room needs 1500 Wh drawn at 80%.
  EXPECT_DOUBLE_EQ(storage.MaxChargePower(kHour).value, 1500.0);
  // 1200 Wh available delivers 600 Wh at 50%.
  EXPECT_DOUBLE_EQ(storage.MaxDischargePower(kHour).value, 600.0);

  storage.SetPowerCommand(PowerCommand::kCharge, Power(500.0), kHour);
  EXPECT_DOUBLE_EQ(storage.stored_energy().value_or(Energy(0.0)).value,
                   1500.0 + 400.0);
  storage.SetPowerCommand(PowerCommand::kDischarge, Power(100.0), kHour);
  EXPECT_DOUBLE_EQ(storage.stored_energy().value_or(Energy(0.0)).value,
                   1900.0 - 200.0);
}

TEST(BatteryStorageTest, CommandAboveSocLimitLeavesStateUnchanged) {
  BatteryStorage storage = MakeWithModel(Model());
  EXPECT_THROW(
      storage.SetPowerCommand(PowerCommand::kCharge, Power(1300.0), kHour),
      OverloadError);
  EXPECT_THROW(
      storage.SetPowerCommand(PowerCommand::kDischarge, Power(1300.0), kHour),
      OverloadError);
  EXPECT_DOUBLE_EQ(storage.power_command().value, 0.0);
  EXPECT_DOUBLE_EQ(storage.inverter().power().value, 0.0);
  EXPECT_DOUBLE_EQ(storage.state_of_charge().value_or(0.0), 0.5);
}

TEST(BatteryStorageTest, ChargingToLimitReachesMaxSoc) {
  BatteryStorage storage = MakeWithModel(Model());
  storage.SetPowerCommand(PowerCommand::kCharge, storage.MaxChargePower(kHour),
                          kHour);
  EXPECT_NEAR(storage.state_of_charge().value_or(0.0), 0.9, 1e-12);
  EXPECT_DOUBLE_EQ(storage.MaxChargePower(kHour).value, 0.0);
}

TEST(BatteryStorageTest, RejectsInvalidStep) {
  BatteryStorage storage = MakeStorage();
  for (const double hours :
       {0.0, -1.0, std::numeric_limits<double>::quiet_NaN()}) {
    EXPECT_THROW((void)storage.MaxChargePower(Hours(hours)),
                 InvalidParameterError);
    EXPECT_THROW((void)storage.MaxDischargePower(Hours(hours)),
                 InvalidParameterError);
    EXPECT_THROW(storage.SetPowerCommand(PowerCommand::kCharge, Power(10.0),
                                         Hours(hours)),
                 InvalidParameterError);
  }
  EXPECT_DOUBLE_EQ(storage.power_command().value, 0.0);
}

TEST(BatteryStorageTest, RejectsInvalidModel) {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  auto expect_invalid = [](const StateOfChargeModel& model) {
    EXPECT_THROW(MakeWithModel(model), InvalidParameterError);
  };
  StateOfChargeModel m = Model();
  m.module_capacity = Energy(0.0);
  expect_invalid(m);
  m = Model();
  m.module_capacity = Energy(nan);
  expect_invalid(m);
  m = Model();
  m.min_soc = 0.9;  // Not below max_soc.
  expect_invalid(m);
  m = Model();
  m.min_soc = -0.1;
  expect_invalid(m);
  m = Model();
  m.max_soc = 1.1;
  expect_invalid(m);
  m = Model();
  m.initial_soc = 0.05;  // Below min_soc.
  expect_invalid(m);
  m = Model();
  m.initial_soc = nan;
  expect_invalid(m);
  m = Model();
  m.charge_efficiency = 0.0;
  expect_invalid(m);
  m = Model();
  m.discharge_efficiency = 1.5;
  expect_invalid(m);
}

}  // namespace
}  // namespace home_ems
