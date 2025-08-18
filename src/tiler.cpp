#include <gdal.h>
#include <gdal_priv.h>
#include <iostream>
#include "tiler_primitives.cpp"

using namespace std;

typedef struct
{
  GDALDataset *dataset;
  int xSize;
  int ySize;
  int bands;
  GDALDataType dataType;
  void *buffer;
  double *geoTransform;
  Extent extent;
} ReadDataset;

void *allocateGridBuffer(GDALDataType eType, int nXSize, int nYSize)
{
  void *gridBuffer = nullptr;

  switch (eType)
  {
  case GDT_UInt16:
    gridBuffer = new uint16_t[nXSize * nYSize];
    break;
  case GDT_Int16:
    gridBuffer = new int16_t[nXSize * nYSize];
    break;
  case GDT_Float32:
    gridBuffer = new float[nXSize * nYSize];
    break;
  case GDT_Float64:
    gridBuffer = new double[nXSize * nYSize];
    break;
  case GDT_Byte:
    gridBuffer = new uint8_t[nXSize * nYSize];
    break;
  case GDT_UInt32:
    gridBuffer = new uint32_t[nXSize * nYSize];
    break;
  case GDT_Int32:
    gridBuffer = new int32_t[nXSize * nYSize];
    break;
  default:
    std::cerr << "[Error]: unsupported data type" << std::endl;
    exit(1);
  }

  if (gridBuffer == NULL)
  {
    std::cerr << "[Error]: cannot allocate memory" << std::endl;
    exit(1);
  }

  return gridBuffer;
}

/**
 * @brief Get the GeoTransform coefficients of a GDAL dataset.
 *
 * A geotransform consists of 6 coefficients:
 *  - GT(0): x-coordinate of the upper-left corner of the upper-left pixel.
 *  - GT(1): w-e pixel resolution / pixel width.
 *  - GT(2): row rotation (typically zero).
 *  - GT(3): y-coordinate of the upper-left corner of the upper-left pixel.
 *  - GT(4): column rotation (typically zero).
 *  - GT(5): n-s pixel resolution / pixel height (negative value for a north-up image).
 *
 * @param poDataset Pointer to the GDALDataset.
 * @return double* Pointer to the array of 6 geotransform coefficients.
 */
double *getGeoTransform(GDALDataset *poDataset)
{
  double *adfGeoTransform = (double *)malloc(sizeof(double) * 6);

  if (poDataset->GetGeoTransform(adfGeoTransform) != CE_None)
  {
    std::cerr << "Error: cannot get GeoTransform" << std::endl;
    exit(1);
  }
  // verify dataset is not rotated/skewed
  if (adfGeoTransform[2] != 0 || adfGeoTransform[4] != 0)
  {
    std::cerr << "Error: dataset is rotated or skewed" << std::endl;
    exit(1);
  }
  return adfGeoTransform;
}

void calculateDatasetExtent(ReadDataset &dataset)
{
  double x_a = dataset.geoTransform[0];
  double x_b = dataset.geoTransform[0] + dataset.geoTransform[1] * dataset.xSize;
  double y_a = dataset.geoTransform[3] + dataset.geoTransform[5] * dataset.ySize;
  double y_b = dataset.geoTransform[3];

  dataset.extent.minX = std::min(x_a, x_b);
  dataset.extent.minY = std::min(y_a, y_b);
  dataset.extent.maxX = std::max(x_a, x_b);
  dataset.extent.maxY = std::max(y_a, y_b);
}

ReadDataset readDataset(const char *filename)
{
  GDALAllRegister();
  GDALDataset *poDataset = (GDALDataset *)GDALOpen(filename, GA_ReadOnly);
  if (poDataset == nullptr)
  {
    std::cerr << "Error: cannot open file " << filename << std::endl;
    exit(1);
  }

  ReadDataset result;
  result.dataset = poDataset;
  result.xSize = poDataset->GetRasterXSize();
  result.ySize = poDataset->GetRasterYSize();
  result.bands = poDataset->GetRasterCount();
  result.dataType = poDataset->GetRasterBand(1)->GetRasterDataType();
  result.buffer = allocateGridBuffer(result.dataType, result.xSize, result.ySize);

  if (result.dataset->GetRasterBand(1)->RasterIO(GF_Read, 0, 0, result.xSize, result.ySize, result.buffer, result.xSize, result.ySize, result.dataType, 0, 0) != CE_None)
  {
    std::cout << "ERROR: cannot read data" << std::endl;
    exit(1);
  }

  result.geoTransform = getGeoTransform(result.dataset);
  calculateDatasetExtent(result);

  return result;
}

void getTileGeoTransform(double *tileGeoTransform, ReadDataset &dataset, double tileMinX, double tileMinY, double tileMaxX, double tileMaxY)
{
  double pixelWidth = TILE_PIXEL_SIZE_M;
  double pixelHeight = TILE_PIXEL_SIZE_M;

  // Calculate the top-left corner of the tile
  double tileTopLeftX = tileMinX;
  double tileTopLeftY = tileMaxY;

  // Calculate the bottom-right corner of the tile
  double tileBottomRightX = tileMaxX;
  double tileBottomRightY = tileMinY;

  // Create a new GeoTransform array for the tile
  if (tileGeoTransform == nullptr)
  {
    std::cerr << "Error: geotransform allocation failed" << std::endl;
    exit(1);
  }

  // Set the GeoTransform coefficients for the tile
  tileGeoTransform[0] = tileTopLeftX;                                  // top-left x
  tileGeoTransform[1] = (tileTopLeftX - tileBottomRightX) / TILE_SIZE; // pixel width
  tileGeoTransform[2] = 0;                                             // rotation (0 for north-up)
  tileGeoTransform[3] = tileTopLeftY;                                  // top-left y
  tileGeoTransform[4] = 0;                                             // rotation (0 for north-up)
  tileGeoTransform[5] = (tileTopLeftY - tileBottomRightY) / TILE_SIZE; // pixel height (negative value for north-up)
}

void writeTileToDisk(void *tileBuffer, const char *filename, GDALDataType dataType, double *geoTransform, const char *projectionRef)
{
  GDALDriver *poDriverMEM = GetGDALDriverManager()->GetDriverByName("MEM");
  GDALDriver *poDriverHF = GetGDALDriverManager()->GetDriverByName("HF2");
  if (poDriverMEM == NULL || poDriverHF == NULL)
  {
    std::cerr << "Error: cannot get driver" << std::endl;
    exit(1);
  }
  // creation options for all tiles
  char **papszOptions = NULL;
  papszOptions = CSLSetNameValue(papszOptions, "COMPRESS", "NO");
  papszOptions = CSLSetNameValue(papszOptions, "VERTICAL_PRECISION", "0.1");

  GDALDataset *poDstDatasetTmp;

  poDstDatasetTmp = poDriverMEM->Create("", TILE_SIZE, TILE_SIZE, 1, dataType, NULL);

  if (poDstDatasetTmp->GetRasterBand(1)->RasterIO(GF_Write, 0, 0, TILE_SIZE, TILE_SIZE, tileBuffer, TILE_SIZE, TILE_SIZE, dataType, 0, 0) != CE_None)
  {
    std::cerr << "Error: cannot write data" << std::endl;
    exit(1);
  }
  poDstDatasetTmp->SetGeoTransform(geoTransform);
  poDstDatasetTmp->SetProjection(projectionRef);
  GDALDataset *poDstDataset;
  poDstDataset = poDriverHF->CreateCopy(filename, poDstDatasetTmp, FALSE, papszOptions, NULL, NULL);

  GDALClose(poDstDatasetTmp);
  GDALClose(poDstDataset);
}

void *sampleTileFromGrid(ReadDataset &dataset, double tileMinX, double tileMinY, double tileMaxX, double tileMaxY)
{
  void *tileBuffer = allocateGridBuffer(dataset.dataType, TILE_SIZE, TILE_SIZE);

  double pixelWidth = dataset.geoTransform[1];
  double pixelHeight = dataset.geoTransform[5];
  float startGridIndexX = static_cast<int>((tileMinX - dataset.geoTransform[0]) / pixelWidth);
  float startGridIndexY = static_cast<int>((tileMinY - dataset.geoTransform[3]) / pixelHeight);
  float endGridIndexX = static_cast<int>((tileMaxX - dataset.geoTransform[0]) / pixelWidth);
  float endGridIndexY = static_cast<int>((tileMaxY - dataset.geoTransform[3]) / pixelHeight);

  for (int j = 0; j < TILE_SIZE; ++j)
  {
    for (int i = 1; i <= TILE_SIZE; ++i)
    {
      // Compute the grid indices for the current tile pixel
      // In Future, adapt to nearest-neighbor-resampling
      int gridIndexX = static_cast<int>(startGridIndexX + i * (endGridIndexX - startGridIndexX) / TILE_SIZE);
      int gridIndexY = static_cast<int>(startGridIndexY + j * (endGridIndexY - startGridIndexY) / TILE_SIZE);

      if (gridIndexX < 0 || gridIndexX >= dataset.xSize || gridIndexY < 0 || gridIndexY >= dataset.ySize)
      {
        // Out of bounds, fill with default value
        switch (dataset.dataType)
        {
        case GDT_Float32:
          reinterpret_cast<float *>(tileBuffer)[(j + 1) * TILE_SIZE - i] = 9999.0f;
          break;
        case GDT_Float64:
          reinterpret_cast<double *>(tileBuffer)[(j + 1) * TILE_SIZE - i] = 9999.0;
          break;
        case GDT_Byte:
          reinterpret_cast<uint8_t *>(tileBuffer)[(j + 1) * TILE_SIZE - i] = 0;
          break;
        case GDT_UInt32:
          reinterpret_cast<uint32_t *>(tileBuffer)[(j + 1) * TILE_SIZE - i] = 9999;
          break;
        case GDT_Int32:
          reinterpret_cast<int32_t *>(tileBuffer)[(j + 1) * TILE_SIZE - i] = 9999;
          break;
        case GDT_Int16:
          reinterpret_cast<int16_t *>(tileBuffer)[(j + 1) * TILE_SIZE - i] = 9999;
          break;
        case GDT_UInt16:
          reinterpret_cast<uint16_t *>(tileBuffer)[(j + 1) * TILE_SIZE - i] = 9999;
          break;
        default:
          std::cerr << "[Error]: unsupported data type" << std::endl;
          exit(1);
        }
      }
      else
      {
        // Sample the value from the grid buffer
        int gridIndex = gridIndexY * dataset.xSize + gridIndexX;

        switch (dataset.dataType)
        {
        case GDT_Float32:
          reinterpret_cast<float *>(tileBuffer)[(j + 1) * TILE_SIZE - i] = reinterpret_cast<float *>(dataset.buffer)[gridIndex];
          break;
        case GDT_Float64:
          reinterpret_cast<double *>(tileBuffer)[(j + 1) * TILE_SIZE - i] = reinterpret_cast<double *>(dataset.buffer)[gridIndex];
          break;
        case GDT_Byte:
          reinterpret_cast<uint8_t *>(tileBuffer)[(j + 1) * TILE_SIZE - i] = reinterpret_cast<uint8_t *>(dataset.buffer)[gridIndex];
          break;
        case GDT_UInt32:
          reinterpret_cast<uint32_t *>(tileBuffer)[(j + 1) * TILE_SIZE - i] = reinterpret_cast<uint32_t *>(dataset.buffer)[gridIndex];
          break;
        case GDT_Int32:
          reinterpret_cast<int32_t *>(tileBuffer)[(j + 1) * TILE_SIZE - i] = reinterpret_cast<int32_t *>(dataset.buffer)[gridIndex];
          break;
        case GDT_Int16:
          reinterpret_cast<int16_t *>(tileBuffer)[(j + 1) * TILE_SIZE - i] = reinterpret_cast<int16_t *>(dataset.buffer)[gridIndex];
          break;
        case GDT_UInt16:
          reinterpret_cast<uint16_t *>(tileBuffer)[(j + 1) * TILE_SIZE - i] = reinterpret_cast<uint16_t *>(dataset.buffer)[gridIndex];
          break;
        default:
          std::cerr << "[Error]: unsupported data type" << std::endl;
          exit(1);
        }
      }
    }
  }

  return tileBuffer;
}

void progressReporting(int current, int total)
{
  int barWidth = 70;
  float progress = static_cast<float>(current) / total;
  int pos = static_cast<int>(barWidth * progress);

  std::cout << "[";
  for (int i = 0; i < barWidth; ++i)
  {
    if (i < pos)
      std::cout << "=";
    else
      std::cout << " ";
  }
  std::cout << "] " << static_cast<int>(progress * 100) << "%\r";
  std::cout.flush();
}

int main(int argc, char *argv[])
{
  GDALAllRegister();
  ReadDataset dataset = readDataset(argv[1]);

  cout << "Dataset read successfully." << endl;

  // find upper left corner
  double upperLeftX = dataset.geoTransform[0];
  double upperLeftY = dataset.geoTransform[3];
  double pixelWidth = dataset.geoTransform[1];
  double pixelHeight = dataset.geoTransform[5];
  double absPixelWidth = std::abs(pixelWidth);
  double absPixelHeight = std::abs(pixelHeight);
  double lowerRightX = upperLeftX + (pixelWidth * dataset.xSize);
  double lowerRightY = upperLeftY + (pixelHeight * dataset.ySize);

  const char *projection = dataset.dataset->GetProjectionRef();
  if (projection != nullptr)
  {
    std::cout << "Check if your Projection has unit m (meteres): " << projection << std::endl;
  }

  // find number of WMTS Tiles required
  double sizeMetersX = absPixelWidth * dataset.xSize;
  double sizeMetersY = absPixelHeight * dataset.ySize;
  int numTilesX = static_cast<int>(std::ceil(sizeMetersX / (TILE_SIZE * TILE_PIXEL_SIZE_M)));
  int numTilesY = static_cast<int>(std::ceil(sizeMetersY / (TILE_SIZE * TILE_PIXEL_SIZE_M)));
  std::cout << "Number of WMTS Tiles: " << numTilesX << " x " << numTilesY << std::endl;

  // find tile size in meters
  double tileSizeMetersX = TILE_SIZE * TILE_PIXEL_SIZE_M;
  double tileSizeMetersY = TILE_SIZE * TILE_PIXEL_SIZE_M;

  for (int j = 0; j < numTilesY; ++j)
  {
    for (int i = 0; i < numTilesX; ++i)
    {
      double tileMinX = pixelWidth > 0 ? upperLeftX + (i * tileSizeMetersX) : upperLeftX - (i * tileSizeMetersX);
      double tileMinY = pixelHeight > 0 ? upperLeftY + (j * tileSizeMetersY) : upperLeftY - (j * tileSizeMetersY);
      double tileMaxX = pixelWidth > 0 ? tileMinX + tileSizeMetersX : tileMinX - tileSizeMetersX;
      double tileMaxY = pixelHeight > 0 ? tileMinY + tileSizeMetersY : tileMinY - tileSizeMetersY;

      std::cout << "Tile " << i << ", " << j << ": "
                << tileMinX << ", " << tileMinY << " - "
                << tileMaxX << ", " << tileMaxY << std::endl;
      void *tileData =
          sampleTileFromGrid(dataset, tileMinX, tileMinY, tileMaxX, tileMaxY);
      char filename[256];
      snprintf(filename, sizeof(filename), "tiles/tile_%d_%d.hf2", i, j);
      double *geoTransform = (double *)malloc(sizeof(double) * 6);
      if (geoTransform == nullptr)
      {
        std::cerr << "Error: cannot allocate memory for GeoTransform" << std::endl;
        exit(1);
      }
      getTileGeoTransform(geoTransform, dataset, tileMinX, tileMinY, tileMaxX, tileMaxY);
      writeTileToDisk(tileData, filename, dataset.dataType, geoTransform, projection);

      free(tileData);
      free(geoTransform);

      progressReporting(j * numTilesX + i + 1, numTilesX * numTilesY);
    }
  }

  // cleanup
  free(dataset.geoTransform);
  free(dataset.buffer);
  GDALClose(dataset.dataset);
}