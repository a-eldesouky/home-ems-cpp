#include "home_ems/grid.h"

#include <limits>

#include <gtest/gtest.h>

#include "home_ems/errors.h"
#include "home_ems/units.h"
#include "test_helpers.h"

namespace home_ems {
namespace {

using testing::MakeGrid;

TEST(GridTest, SellAndBuyEnergy) {
  Grid grid = MakeGrid();
  grid.SellEnergy(Energy(1000.0));
  grid.BuyEnergy(Energy(500.0));
  EXPECT_DOUBLE_EQ(grid.sold_energy().value, 1000.0);
  EXPECT_DOUBLE_EQ(grid.bought_energy().value, 500.0);
}

TEST(GridTest, TotalsAccumulate) {
  Grid grid = MakeGrid();
  grid.SellEnergy(Energy(100.0));
  grid.SellEnergy(Energy(250.0));
  grid.BuyEnergy(Energy(40.0));
  grid.BuyEnergy(Energy(60.0));
  EXPECT_DOUBLE_EQ(grid.sold_energy().value, 350.0);
  EXPECT_DOUBLE_EQ(grid.bought_energy().value, 100.0);
}

TEST(GridTest, StoresVoltageAndFrequency) {
  const Grid grid = MakeGrid();
  EXPECT_DOUBLE_EQ(grid.voltage().value, 230.0);
  EXPECT_DOUBLE_EQ(grid.frequency().value, 50.0);
  EXPECT_DOUBLE_EQ(grid.sold_energy().value, 0.0);
  EXPECT_DOUBLE_EQ(grid.bought_energy().value, 0.0);
}

TEST(GridTest, SellZeroEnergy) {
  Grid grid = MakeGrid();
  EXPECT_NO_THROW(grid.SellEnergy(Energy(0.0)));
  EXPECT_DOUBLE_EQ(grid.sold_energy().value, 0.0);
}

TEST(GridTest, BuyZeroEnergy) {
  Grid grid = MakeGrid();
  EXPECT_NO_THROW(grid.BuyEnergy(Energy(0.0)));
  EXPECT_DOUBLE_EQ(grid.bought_energy().value, 0.0);
}

TEST(GridTest, RejectsSellingNegativeEnergy) {
  Grid grid = MakeGrid();
  EXPECT_THROW(grid.SellEnergy(Energy(-1000.0)), InvalidParameterError);
  EXPECT_DOUBLE_EQ(grid.sold_energy().value, 0.0);
}

TEST(GridTest, RejectsBuyingNegativeEnergy) {
  Grid grid = MakeGrid();
  EXPECT_THROW(grid.BuyEnergy(Energy(-1.0)), InvalidParameterError);
  EXPECT_DOUBLE_EQ(grid.bought_energy().value, 0.0);
}

TEST(GridTest, RejectsNonFiniteEnergy) {
  Grid grid = MakeGrid();
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const double inf = std::numeric_limits<double>::infinity();
  EXPECT_THROW(grid.SellEnergy(Energy(nan)), InvalidParameterError);
  EXPECT_THROW(grid.BuyEnergy(Energy(nan)), InvalidParameterError);
  EXPECT_THROW(grid.SellEnergy(Energy(inf)), InvalidParameterError);
  EXPECT_DOUBLE_EQ(grid.sold_energy().value, 0.0);
  EXPECT_DOUBLE_EQ(grid.bought_energy().value, 0.0);
}

}  // namespace
}  // namespace home_ems
