#include <gtest/gtest.h>
#include "../src/tiler_primitives.cpp"

// Example test case
TEST(TileTest, TileExtentCalculation)
{
  Extent parentExtent = {0, 0, 1000, 1000};
  TileProductionReq req = prepareTile(parentExtent, 0, 0);
  EXPECT_DOUBLE_EQ(req.extent.minX, 0.0);
  EXPECT_DOUBLE_EQ(req.extent.minY, 0.0);
  EXPECT_DOUBLE_EQ(req.extent.maxX, 2560);
  EXPECT_DOUBLE_EQ(req.extent.maxY, 2560);
  EXPECT_EQ(req.tileIndexX, 0);
  EXPECT_EQ(req.tileIndexY, 0);
}

TEST(TileTest, TileGeoTransformCalculation)
{
  Extent parentExtent = {0, 0, 1000, 1000};
  TileProductionReq req = prepareTile(parentExtent, 0, 0);
  EXPECT_NE(req.geoTransform, nullptr);
  EXPECT_DOUBLE_EQ(req.geoTransform[0], 0.0);
  EXPECT_DOUBLE_EQ(req.geoTransform[1], 10.0);
  EXPECT_DOUBLE_EQ(req.geoTransform[3], 2560.0);
  EXPECT_DOUBLE_EQ(req.geoTransform[5], -10.0);
}