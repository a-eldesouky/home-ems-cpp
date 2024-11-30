#include "cli_options.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "home_ems/errors.h"

namespace home_ems::cli {
namespace {

TEST(ParseCliOptionsTest, ProfileOnlyUsesDefaults) {
  const CliOptions options = ParseCliOptions({"day.csv"});
  EXPECT_EQ(options.profile_path, "day.csv");
  EXPECT_DOUBLE_EQ(options.inverter_max_w, kDefaultInverterMaxW);
  EXPECT_FALSE(options.show_help);
}

TEST(ParseCliOptionsTest, ReadsInverterLimit) {
  const CliOptions options =
      ParseCliOptions({"--inverter-max-w", "2500.5", "day.csv"});
  EXPECT_DOUBLE_EQ(options.inverter_max_w, 2500.5);
  EXPECT_EQ(options.profile_path, "day.csv");
}

TEST(ParseCliOptionsTest, HelpWinsOverEverythingElse) {
  EXPECT_TRUE(ParseCliOptions({"--help"}).show_help);
  EXPECT_TRUE(ParseCliOptions({"--bogus", "-h"}).show_help);
}

TEST(ParseCliOptionsTest, RejectsMissingProfile) {
  EXPECT_THROW(ParseCliOptions({}), InvalidParameterError);
}

TEST(ParseCliOptionsTest, RejectsTwoProfiles) {
  EXPECT_THROW(ParseCliOptions({"a.csv", "b.csv"}), InvalidParameterError);
}

TEST(ParseCliOptionsTest, RejectsUnknownOption) {
  EXPECT_THROW(ParseCliOptions({"--verbose", "a.csv"}), InvalidParameterError);
}

TEST(ParseCliOptionsTest, RejectsMissingValue) {
  EXPECT_THROW(ParseCliOptions({"a.csv", "--inverter-max-w"}),
               InvalidParameterError);
}

TEST(ParseCliOptionsTest, RejectsInvalidInverterLimit) {
  for (const std::string value :
       {"0", "-1", "abc", "", "inf", "nan", "1e999", " 5", "5 ", "0x10"}) {
    EXPECT_THROW(ParseCliOptions({"--inverter-max-w", value, "a.csv"}),
                 InvalidParameterError)
        << "value: '" << value << "'";
  }
}

TEST(UsageTextTest, NamesProgramAndOptions) {
  const std::string text = UsageText("home-ems");
  EXPECT_NE(text.find("Usage: home-ems"), std::string::npos);
  EXPECT_NE(text.find("--inverter-max-w"), std::string::npos);
}

TEST(ParseCliOptionsTest, ReadsStepMinutes) {
  EXPECT_EQ(ParseCliOptions({"--step-minutes", "15", "a.csv"}).step_minutes,
            15);
  EXPECT_EQ(ParseCliOptions({"a.csv"}).step_minutes, kDefaultStepMinutes);
  for (const std::string value : {"0", "1441", "2.5", "-5", "x"}) {
    EXPECT_THROW(ParseCliOptions({"--step-minutes", value, "a.csv"}),
                 InvalidParameterError)
        << value;
  }
}

TEST(ParseCliOptionsTest, ReadsStateOfChargeOptions) {
  const CliOptions o = ParseCliOptions(
      {"--capacity-wh", "5000", "--soc-min", "20", "--soc-max", "80",
       "--soc-initial", "30", "--efficiency", "0.9", "a.csv"});
  EXPECT_DOUBLE_EQ(o.capacity_wh.value_or(0.0), 5000.0);
  EXPECT_DOUBLE_EQ(o.soc_min_pct, 20.0);
  EXPECT_DOUBLE_EQ(o.soc_max_pct, 80.0);
  EXPECT_DOUBLE_EQ(o.soc_initial_pct, 30.0);
  EXPECT_DOUBLE_EQ(o.efficiency, 0.9);
}

TEST(ParseCliOptionsTest, NoCapacityMeansNoStateOfCharge) {
  EXPECT_FALSE(ParseCliOptions({"a.csv"}).capacity_wh.has_value());
}

TEST(ParseCliOptionsTest, SocOptionsNeedCapacity) {
  EXPECT_THROW(ParseCliOptions({"--soc-min", "20", "a.csv"}),
               InvalidParameterError);
  EXPECT_THROW(ParseCliOptions({"--efficiency", "0.9", "a.csv"}),
               InvalidParameterError);
}

TEST(ParseCliOptionsTest, RejectsInconsistentSocLimits) {
  EXPECT_THROW(ParseCliOptions({"--capacity-wh", "1000", "--soc-min", "50",
                                "--soc-max", "50", "a.csv"}),
               InvalidParameterError);
  EXPECT_THROW(
      ParseCliOptions({"--capacity-wh", "1000", "--soc-initial", "5", "a.csv"}),
      InvalidParameterError);
}

TEST(ParseCliOptionsTest, RejectsInvalidSocValues) {
  for (const auto& args : std::vector<std::vector<std::string>>{
           {"--capacity-wh", "0", "a.csv"},
           {"--capacity-wh", "1000", "--soc-min", "-1", "a.csv"},
           {"--capacity-wh", "1000", "--soc-max", "101", "a.csv"},
           {"--capacity-wh", "1000", "--soc-initial", "nan", "a.csv"},
           {"--capacity-wh", "1000", "--efficiency", "0", "a.csv"},
           {"--capacity-wh", "1000", "--efficiency", "1.01", "a.csv"},
           {"--capacity-wh"}}) {
    EXPECT_THROW(ParseCliOptions(args), InvalidParameterError);
  }
}

}  // namespace
}  // namespace home_ems::cli
