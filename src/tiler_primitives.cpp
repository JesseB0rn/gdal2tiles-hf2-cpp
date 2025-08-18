#define TILE_SIZE 256
#define TILE_PIXEL_SIZE_M 10.0

#include <iostream>

typedef struct
{
  double minX;
  double minY;
  double maxX;
  double maxY;
} Extent;

typedef struct
{
  int tileIndexX;
  int tileIndexY;
  double *geoTransform;
  Extent extent;
} TileProductionReq;

static TileProductionReq prepareTile(Extent parentDatasetExtent, int tileX, int tileY)
{
  TileProductionReq req;

  req.tileIndexX = tileX;
  req.tileIndexY = tileY;

  // find tile extent
  double tileMinX = parentDatasetExtent.minX + tileX * TILE_SIZE * TILE_PIXEL_SIZE_M;
  double tileMinY = parentDatasetExtent.minY + tileY * TILE_SIZE * TILE_PIXEL_SIZE_M;
  double tileMaxX = tileMinX + double(TILE_SIZE) * TILE_PIXEL_SIZE_M;
  double tileMaxY = tileMinY + double(TILE_SIZE) * TILE_PIXEL_SIZE_M;

  req.extent.minX = tileMinX;
  req.extent.minY = tileMinY;
  req.extent.maxX = tileMaxX;
  req.extent.maxY = tileMaxY;

  // assumption: tile and source image are both north up

  double *adfTileGeoTransform = (double *)malloc(sizeof(double) * 6);

  if (adfTileGeoTransform == NULL)
  {
    std::cerr << "[Error]: cannot allocate memory for tile GeoTransform" << std::endl;
    exit(1);
  }

  adfTileGeoTransform[0] = tileMinX;
  adfTileGeoTransform[1] = TILE_PIXEL_SIZE_M;
  adfTileGeoTransform[2] = 0;
  adfTileGeoTransform[3] = tileMaxY;
  adfTileGeoTransform[4] = 0;
  adfTileGeoTransform[5] = -TILE_PIXEL_SIZE_M;

  req.geoTransform = adfTileGeoTransform;

  return req;
}