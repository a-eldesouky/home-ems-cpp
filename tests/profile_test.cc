#include "home_ems/profile.h"

#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "home_ems/errors.h"

namespace home_ems {
namespace {

std::vector<ProfileSample> Parse(const std::string& text) {
  std::istringstream in(text);
  return ReadProfile(in);
}

// Returns the error message ReadProfile throws for `text`, or "" if none.
std::string ParseError(const std::string& text) {
  try {
    (void)Parse(text);
  } catch (const InvalidParameterError& e) {
    return e.what();
  }
  return "";
}

TEST(ReadProfileTest, ReadsRows) {
  const auto samples =
      Parse("time,pv_w,load_w\n00:00,0,350\n12:00,6100.5,400\n");
  ASSERT_EQ(samples.size(), 2U);
  EXPECT_EQ(samples[0].time, "00:00");
  EXPECT_DOUBLE_EQ(samples[0].pv.value, 0.0);
  EXPECT_DOUBLE_EQ(samples[0].load.value, 350.0);
  EXPECT_EQ(samples[1].time, "12:00");
  EXPECT_DOUBLE_EQ(samples[1].pv.value, 6100.5);
}

TEST(ReadProfileTest, AcceptsCrlfAndBlankLines) {
  const auto samples =
      Parse("time,pv_w,load_w\r\n\r\n01:00,10,20\r\n\n02:00,30,40");
  ASSERT_EQ(samples.size(), 2U);
  EXPECT_DOUBLE_EQ(samples[1].load.value, 40.0);
}

TEST(ReadProfileTest, HeaderOnlyGivesNoSamples) {
  EXPECT_TRUE(Parse("time,pv_w,load_w\n").empty());
}

TEST(ReadProfileTest, EmptyInputFails) {
  EXPECT_EQ(ParseError(""), "line 1: missing header 'time,pv_w,load_w'");
}

TEST(ReadProfileTest, WrongHeaderFails) {
  EXPECT_EQ(ParseError("t,pv,load\n"),
            "line 1: expected header 'time,pv_w,load_w', got 't,pv,load'");
}

TEST(ReadProfileTest, WrongFieldCountFails) {
  EXPECT_EQ(ParseError("time,pv_w,load_w\n00:00,1\n"),
            "line 2: expected 3 fields, got 2");
  EXPECT_EQ(ParseError("time,pv_w,load_w\n00:00,1,2,3\n"),
            "line 2: expected 3 fields, got 4");
}

TEST(ReadProfileTest, EmptyTimeFails) {
  EXPECT_EQ(ParseError("time,pv_w,load_w\n,1,2\n"), "line 2: time is empty");
}

TEST(ReadProfileTest, EmptyValueFails) {
  EXPECT_EQ(ParseError("time,pv_w,load_w\n00:00,,2\n"),
            "line 2: pv_w is empty");
}

TEST(ReadProfileTest, NonNumericValueFails) {
  EXPECT_EQ(ParseError("time,pv_w,load_w\n00:00,1,abc\n"),
            "line 2: load_w is not a number: 'abc'");
  EXPECT_EQ(ParseError("time,pv_w,load_w\n00:00,12x,1\n"),
            "line 2: pv_w is not a number: '12x'");
}

TEST(ReadProfileTest, NegativeValueFails) {
  EXPECT_EQ(ParseError("time,pv_w,load_w\n00:00,1,2\n01:00,-5,2\n"),
            "line 3: pv_w must be >= 0");
}

TEST(ReadProfileTest, NonFiniteValueFails) {
  EXPECT_EQ(ParseError("time,pv_w,load_w\n00:00,nan,2\n"),
            "line 2: pv_w is not finite");
  EXPECT_EQ(ParseError("time,pv_w,load_w\n00:00,1,inf\n"),
            "line 2: load_w is not finite");
}

TEST(ReadProfileTest, WhitespaceAndHexFail) {
  EXPECT_EQ(ParseError("time,pv_w,load_w\n00:00, 5,2\n"),
            "line 2: pv_w is not a number: ' 5'");
  EXPECT_EQ(ParseError("time,pv_w,load_w\n00:00,5 ,2\n"),
            "line 2: pv_w is not a number: '5 '");
  EXPECT_EQ(ParseError("time,pv_w,load_w\n00:00,0x10,2\n"),
            "line 2: pv_w is not a number: '0x10'");
}

TEST(ReadProfileTest, TinyValueReadsAsZero) {
  const auto samples = Parse("time,pv_w,load_w\n00:00,1e-400,2\n");
  ASSERT_EQ(samples.size(), 1U);
  EXPECT_DOUBLE_EQ(samples[0].pv.value, 0.0);
}

TEST(ReadProfileTest, OutOfRangeValueFails) {
  EXPECT_EQ(ParseError("time,pv_w,load_w\n00:00,1e999,2\n"),
            "line 2: pv_w is not a number: '1e999'");
}

TEST(ReadProfileTest, RecordsLineNumbers) {
  const auto samples = Parse("time,pv_w,load_w\n\n00:00,1,2\n01:00,3,4\n");
  ASSERT_EQ(samples.size(), 2U);
  EXPECT_EQ(samples[0].line, 3);
  EXPECT_EQ(samples[1].line, 4);
}

TEST(ParseClockMinutesTest, ReadsHoursAndMinutes) {
  EXPECT_EQ(ParseClockMinutes("00:00"), 0);
  EXPECT_EQ(ParseClockMinutes("07:30"), 450);
  EXPECT_EQ(ParseClockMinutes("23:59"), 1439);
}

TEST(ParseClockMinutesTest, RejectsOtherText) {
  for (const char* text : {"24:00", "12:60", "7:30", "07:3", "0730", "07-30",
                           "ab:cd", "", "night"}) {
    EXPECT_EQ(ParseClockMinutes(text), std::nullopt) << text;
  }
}

std::string SpacingError(const std::string& text, int step_minutes) {
  try {
    CheckTimeSpacing(Parse(text), step_minutes);
  } catch (const InvalidParameterError& e) {
    return e.what();
  }
  return "";
}

TEST(CheckTimeSpacingTest, AcceptsEvenSpacingAcrossMidnight) {
  EXPECT_EQ(
      SpacingError("time,pv_w,load_w\n23:30,0,1\n23:45,0,1\n00:00,0,1\n", 15),
      "");
}

TEST(CheckTimeSpacingTest, RejectsUnevenSpacing) {
  EXPECT_EQ(
      SpacingError("time,pv_w,load_w\n00:00,0,1\n01:00,0,1\n03:00,0,1\n", 60),
      "line 4: time '03:00' is not 60 minutes after '01:00'");
}

TEST(CheckTimeSpacingTest, RejectsWrongStepLength) {
  EXPECT_EQ(SpacingError("time,pv_w,load_w\n00:00,0,1\n01:00,0,1\n", 30),
            "line 3: time '01:00' is not 30 minutes after '00:00'");
}

TEST(CheckTimeSpacingTest, RejectsMixedLabels) {
  EXPECT_EQ(SpacingError("time,pv_w,load_w\n00:00,0,1\nnoon,0,1\n", 60),
            "line 3: time 'noon' is not HH:MM");
}

TEST(CheckTimeSpacingTest, SkipsFreeFormLabels) {
  EXPECT_EQ(SpacingError("time,pv_w,load_w\nnight,0,1\nnoon,0,1\n", 60), "");
  EXPECT_EQ(SpacingError("time,pv_w,load_w\n", 60), "");
}

TEST(CheckTimeSpacingTest, DailyStepAllowsSameClockTime) {
  EXPECT_EQ(SpacingError("time,pv_w,load_w\n12:00,0,1\n12:00,0,1\n", 1440), "");
}

TEST(CheckTimeSpacingTest, RejectsInvalidStep) {
  EXPECT_EQ(SpacingError("time,pv_w,load_w\n", 0),
            "Step length must be 1 to 1440 minutes.");
  EXPECT_EQ(SpacingError("time,pv_w,load_w\n", 1441),
            "Step length must be 1 to 1440 minutes.");
}

}  // namespace
}  // namespace home_ems
