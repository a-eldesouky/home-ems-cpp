#include "home_ems/energy_system.h"

#include <cstddef>
#include <limits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "home_ems/battery_module.h"
#include "home_ems/errors.h"
#include "home_ems/units.h"
#include "test_helpers.h"

namespace home_ems {
namespace {

TEST(EnergySystemTest, DefaultConfigRuns) {
  EnergySystem system(SystemConfig{});
  ControlResult r;
  EXPECT_NO_THROW(r = system.Run(testing::kHour));
  EXPECT_TRUE(r.error.empty());
  // Defaults: 5000 W PV, 3000 W load, 4000 W inverter.
  EXPECT_DOUBLE_EQ(r.net.value, 2000.0);
  EXPECT_DOUBLE_EQ(r.battery_command.value, 2000.0);
  EXPECT_EQ(system.storage().modules().size(), 3U);
  EXPECT_EQ(system.type(), SystemType::kStandard);
}

TEST(EnergySystemTest, ModuleCountFollowsSystemType) {
  SystemConfig config;
  config.type = SystemType::kBasic;
  EXPECT_EQ(EnergySystem(config).storage().modules().size(), 2U);
  config.type = SystemType::kPro;
  EXPECT_EQ(EnergySystem(config).storage().modules().size(), 5U);
}

TEST(EnergySystemTest, InvalidModuleConfigThrows) {
  SystemConfig config;
  config.module_voltage = Voltage(0.0);
  EXPECT_THROW(EnergySystem{config}, InvalidParameterError);
}

TEST(EnergySystemTest, UpdateChangesNextStep) {
  EnergySystem system(SystemConfig{});
  system.Update(Power(500.0), Power(2500.0));
  const ControlResult r = system.Run(testing::kHour);
  EXPECT_DOUBLE_EQ(system.pv_system().power().value, 500.0);
  EXPECT_DOUBLE_EQ(system.house().power_consumption().value, 2500.0);
  EXPECT_DOUBLE_EQ(r.battery_command.value, -2000.0);
}

TEST(EnergySystemTest, InvalidUpdateThrows) {
  EnergySystem system(SystemConfig{});
  EXPECT_THROW(system.Update(Power(-1.0), Power(100.0)), InvalidParameterError);
  EXPECT_THROW(system.Update(Power(100.0),
                             Power(std::numeric_limits<double>::quiet_NaN())),
               InvalidParameterError);
}

TEST(EnergySystemTest, ModuleRatingLimitsDefaultSystemAboveInverter) {
  // Standard system: 3 x 2000 W modules. A 7000 W inverter is limited to
  // 6000 W by the modules.
  SystemConfig config;
  config.inverter_max_power = Power(7000.0);
  EnergySystem system(config);
  system.Update(Power(9000.0), Power(1000.0));
  const ControlResult r = system.Run(testing::kHour);
  EXPECT_DOUBLE_EQ(r.battery_command.value, 6000.0);
  EXPECT_DOUBLE_EQ(r.grid_export.value, 2000.0);
}

TEST(EnergySystemTest, AcceptsCustomComponents) {
  EnergySystem system(SystemType::kBasic, testing::MakePv(9000.0),
                      testing::MakeGrid(), testing::MakeHouse(1000.0),
                      testing::MakeStorage(2500.0, 2));
  const ControlResult r = system.Run(testing::kHour);
  EXPECT_DOUBLE_EQ(r.battery_command.value, 2500.0);
  EXPECT_DOUBLE_EQ(r.grid_export.value, 5500.0);
  EXPECT_DOUBLE_EQ(system.grid().sold_energy().value, 5500.0);
}

TEST(EnergySystemTest, RejectsMoreModulesThanSystemTypeAllows) {
  EXPECT_THROW(EnergySystem(SystemType::kBasic, testing::MakePv(0.0),
                            testing::MakeGrid(), testing::MakeHouse(0.0),
                            testing::MakeStorage(4000.0, 3)),
               InvalidParameterError);
  EXPECT_THROW(EnergySystem(SystemType::kStandard, testing::MakePv(0.0),
                            testing::MakeGrid(), testing::MakeHouse(0.0),
                            testing::MakeStorage(4000.0, 4)),
               InvalidParameterError);
}

TEST(EnergySystemTest, AcceptsUpToModuleCount) {
  for (const auto& [type, count] :
       {std::pair{SystemType::kBasic, 1}, std::pair{SystemType::kBasic, 2},
        std::pair{SystemType::kStandard, 3}, std::pair{SystemType::kPro, 5}}) {
    EXPECT_NO_THROW(EnergySystem(
        type, testing::MakePv(0.0), testing::MakeGrid(),
        testing::MakeHouse(0.0),
        testing::MakeStorage(4000.0, static_cast<std::size_t>(count))));
  }
}

TEST(EnergySystemTest, DefaultConfigPassesSocModelToStorage) {
  SystemConfig config;
  StateOfChargeModel model;
  model.module_capacity = Energy(1000.0);
  model.initial_soc = 0.2;
  config.soc_model = model;
  const EnergySystem system(config);
  EXPECT_DOUBLE_EQ(system.storage().capacity().value_or(Energy(0.0)).value,
                   3000.0);
  EXPECT_DOUBLE_EQ(system.storage().state_of_charge().value_or(0.0), 0.2);
}

}  // namespace
}  // namespace home_ems
