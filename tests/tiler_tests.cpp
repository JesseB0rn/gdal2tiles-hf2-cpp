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

TEST(CoordinateConversionTest, crsCoordToGridIndex)
{
  double *gt = new double[6];
  gt[0] = 0.0;
  gt[1] = 10.0;
  gt[2] = 0.0;
  gt[3] = 2560.0;
  gt[4] = 0.0;
  gt[5] = -10.0;

  int sourceX, sourceY;

  crsCoordToGridIndex(gt, 1280.0, 1280.0, sourceX, sourceY);
  EXPECT_EQ(sourceX, 128);
  EXPECT_EQ(sourceY, 128);

  crsCoordToGridIndex(gt, 0.0, 0.0, sourceX, sourceY);
  EXPECT_EQ(sourceX, 0);
  EXPECT_EQ(sourceY, 256);

  delete[] gt;
}

TEST(CoordinateConversionTest, ConversionInvertability)
{
  double *gt = new double[6];
  gt[0] = 0.0;
  gt[1] = 10.0;
  gt[2] = 0.0;
  gt[3] = 2560.0;
  gt[4] = 0.0;
  gt[5] = -10.0;

  int sourceX, sourceY;
  crsCoordToGridIndex(gt, 1230.0, 1570.0, sourceX, sourceY);

  double pixelX, pixelY;
  gridToCRSCoord(gt, sourceX, sourceY, pixelX, pixelY);

  EXPECT_DOUBLE_EQ(pixelX, 1230.0);
  EXPECT_DOUBLE_EQ(pixelY, 1570.0);

  delete[] gt;
}