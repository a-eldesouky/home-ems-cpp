#include "home_ems/simulation.h"

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "home_ems/battery_storage.h"
#include "home_ems/energy_system.h"
#include "home_ems/profile.h"
#include "home_ems/units.h"
#include "test_helpers.h"

namespace home_ems {
namespace {

using testing::kHour;

std::vector<ProfileSample> Samples() {
  return {
      {"night", Power(0.0), Power(4500.0), 2},   // Shortage above the limit.
      {"noon", Power(6500.0), Power(500.0), 3},  // Surplus above the limit.
      {"even", Power(800.0), Power(800.0), 4},   // Zero net power.
  };
}

TEST(SimulateTest, RunsEverySampleInOrder) {
  EnergySystem system(SystemConfig{});
  const std::vector<StepRecord> records = Simulate(system, Samples(), kHour);

  ASSERT_EQ(records.size(), 3U);
  EXPECT_EQ(records[0].time, "night");
  EXPECT_DOUBLE_EQ(records[0].result.battery_command.value, -4000.0);
  EXPECT_DOUBLE_EQ(records[0].result.grid_import.value, 500.0);
  EXPECT_DOUBLE_EQ(records[1].result.battery_command.value, 4000.0);
  EXPECT_DOUBLE_EQ(records[1].result.grid_export.value, 2000.0);
  EXPECT_DOUBLE_EQ(records[2].result.net.value, 0.0);
  EXPECT_FALSE(records[2].result.state_of_charge.has_value());
  EXPECT_DOUBLE_EQ(system.grid().sold_energy().value, 2000.0);
  EXPECT_DOUBLE_EQ(system.grid().bought_energy().value, 500.0);
}

TEST(SimulateTest, EmptyProfileGivesNoRecords) {
  EnergySystem system(SystemConfig{});
  EXPECT_TRUE(Simulate(system, {}, kHour).empty());
}

TEST(SimulateTest, GridEnergyScalesWithStepLength) {
  EnergySystem system(SystemConfig{});
  (void)Simulate(system, Samples(), Hours(0.25));
  EXPECT_DOUBLE_EQ(system.grid().sold_energy().value, 500.0);
  EXPECT_DOUBLE_EQ(system.grid().bought_energy().value, 125.0);
}

TEST(SummarizeTest, SumsEnergyFlows) {
  EnergySystem system(SystemConfig{});
  const Hours step(0.5);
  const SimulationSummary s =
      Summarize(Simulate(system, Samples(), step), step);
  EXPECT_EQ(s.steps, 3U);
  EXPECT_EQ(s.failed_steps, 0U);
  EXPECT_DOUBLE_EQ(s.grid_export.value, 1000.0);
  EXPECT_DOUBLE_EQ(s.grid_import.value, 250.0);
  EXPECT_DOUBLE_EQ(s.battery_charge.value, 2000.0);
  EXPECT_DOUBLE_EQ(s.battery_discharge.value, 2000.0);
  EXPECT_FALSE(s.final_state_of_charge.has_value());
  // The summary agrees with the grid's own totals.
  EXPECT_DOUBLE_EQ(s.grid_export.value, system.grid().sold_energy().value);
}

TEST(SummarizeTest, CountsFailedStepsAndSkipsTheirFlows) {
  SystemConfig config;
  config.inverter_max_power = Power(0.0);
  EnergySystem system(config);
  const SimulationSummary s =
      Summarize(Simulate(system, Samples(), kHour), kHour);
  EXPECT_EQ(s.failed_steps, 3U);
  EXPECT_DOUBLE_EQ(s.grid_export.value, 0.0);
  EXPECT_DOUBLE_EQ(s.battery_discharge.value, 0.0);
}

TEST(SummarizeTest, ReportsFinalStateOfCharge) {
  SystemConfig config;
  StateOfChargeModel model;
  model.module_capacity = Energy(2000.0);  // 3 modules: 6000 Wh.
  model.initial_soc = 0.5;
  config.soc_model = model;
  EnergySystem system(config);
  const SimulationSummary s = Summarize(
      Simulate(system, {{"t", Power(1600.0), Power(400.0), 2}}, kHour), kHour);
  ASSERT_TRUE(s.final_state_of_charge.has_value());
  EXPECT_NEAR(s.final_state_of_charge.value_or(0.0), 0.7, 1e-12);
}

TEST(WriteResultsCsvTest, WritesHeaderAndRows) {
  EnergySystem system(SystemConfig{});
  std::ostringstream out;
  WriteResultsCsv(out, Simulate(system, Samples(), kHour));
  EXPECT_EQ(out.str(),
            "time,pv_w,load_w,net_w,battery_w,grid_export_w,grid_import_w,"
            "soc_pct,error\n"
            "night,0.0,4500.0,-4500.0,-4000.0,0.0,500.0,,\n"
            "noon,6500.0,500.0,6000.0,4000.0,2000.0,0.0,,\n"
            "even,800.0,800.0,0.0,0.0,0.0,0.0,,\n");
}

TEST(WriteResultsCsvTest, WritesStateOfCharge) {
  SystemConfig config;
  StateOfChargeModel model;
  model.module_capacity = Energy(2000.0);
  config.soc_model = model;
  EnergySystem system(config);
  std::ostringstream out;
  WriteResultsCsv(
      out, Simulate(system, {{"t", Power(1600.0), Power(400.0), 2}}, kHour));
  EXPECT_NE(out.str().find("t,1600.0,400.0,1200.0,1200.0,0.0,0.0,70.0,\n"),
            std::string::npos);
}

TEST(WriteResultsCsvTest, WritesStepError) {
  SystemConfig config;
  config.inverter_max_power = Power(0.0);
  EnergySystem system(config);
  std::ostringstream out;
  WriteResultsCsv(out,
                  Simulate(system, {{"t", Power(10.0), Power(0.0), 2}}, kHour));
  EXPECT_NE(out.str().find("t,10.0,0.0,10.0,0.0,0.0,0.0,,"
                           "Invalid storage max charge power.\n"),
            std::string::npos);
}

TEST(WriteResultsCsvTest, RestoresStreamFormat) {
  std::ostringstream out;
  out << std::setprecision(3);
  WriteResultsCsv(out, {});
  out << 1.23456;
  EXPECT_EQ(out.str().substr(out.str().find('\n') + 1), "1.23");
}

TEST(WriteSummaryTest, WritesTotals) {
  SimulationSummary s;
  s.steps = 4;
  s.failed_steps = 1;
  s.step = Hours(0.25);
  s.grid_export = Energy(1234.56);
  s.grid_import = Energy(-0.0);
  s.battery_charge = Energy(10.0);
  s.battery_discharge = Energy(20.0);
  std::ostringstream out;
  WriteSummary(out, s);
  EXPECT_EQ(out.str(),
            "steps: 4 (failed: 1)\n"
            "step length: 15.0 min\n"
            "grid export: 1234.6 Wh\n"
            "grid import: 0.0 Wh\n"
            "battery charge: 10.0 Wh\n"
            "battery discharge: 20.0 Wh\n");
  s.final_state_of_charge = 0.425;
  std::ostringstream with_soc;
  WriteSummary(with_soc, s);
  EXPECT_NE(with_soc.str().find("final state of charge: 42.5 %\n"),
            std::string::npos);
}

}  // namespace
}  // namespace home_ems
