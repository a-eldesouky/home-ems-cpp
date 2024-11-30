#include "home_ems/parse_number.h"

#include <cmath>
#include <optional>
#include <string>

#include <gtest/gtest.h>

namespace home_ems {
namespace {

TEST(ParseNumberTest, ReadsDecimalNumbers) {
  EXPECT_EQ(ParseNumber("0"), 0.0);
  EXPECT_EQ(ParseNumber("42"), 42.0);
  EXPECT_EQ(ParseNumber("-2.5"), -2.5);
  EXPECT_EQ(ParseNumber("+7"), 7.0);
  EXPECT_EQ(ParseNumber("1.5e3"), 1500.0);
  EXPECT_EQ(ParseNumber(".5"), 0.5);
}

TEST(ParseNumberTest, RejectsNonNumbers) {
  EXPECT_EQ(ParseNumber(""), std::nullopt);
  EXPECT_EQ(ParseNumber("abc"), std::nullopt);
  EXPECT_EQ(ParseNumber("12x"), std::nullopt);
  EXPECT_EQ(ParseNumber("1.2.3"), std::nullopt);
  EXPECT_EQ(ParseNumber("-"), std::nullopt);
}

TEST(ParseNumberTest, RejectsWhitespace) {
  EXPECT_EQ(ParseNumber(" 5"), std::nullopt);
  EXPECT_EQ(ParseNumber("5 "), std::nullopt);
  EXPECT_EQ(ParseNumber("\t5"), std::nullopt);
  EXPECT_EQ(ParseNumber("5\n"), std::nullopt);
}

TEST(ParseNumberTest, RejectsHexadecimal) {
  EXPECT_EQ(ParseNumber("0x10"), std::nullopt);
  EXPECT_EQ(ParseNumber("0X1p3"), std::nullopt);
}

TEST(ParseNumberTest, RejectsEmbeddedNul) {
  EXPECT_EQ(ParseNumber(std::string("5\0"
                                    "1",
                                    3)),
            std::nullopt);
}

TEST(ParseNumberTest, RejectsOverflow) {
  EXPECT_EQ(ParseNumber("1e999"), std::nullopt);
  EXPECT_EQ(ParseNumber("-1e999"), std::nullopt);
}

TEST(ParseNumberTest, KeepsUnderflowAsZero) {
  EXPECT_EQ(ParseNumber("1e-400"), 0.0);
}

TEST(ParseNumberTest, ReturnsInfinityAndNanAsValues) {
  const std::optional<double> inf = ParseNumber("inf");
  EXPECT_TRUE(inf.has_value() && std::isinf(*inf));
  const std::optional<double> nan = ParseNumber("nan");
  EXPECT_TRUE(nan.has_value() && std::isnan(*nan));
}

}  // namespace
}  // namespace home_ems
