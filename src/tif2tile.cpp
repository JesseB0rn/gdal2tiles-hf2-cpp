#include <gdal.h>
#include <gdal_priv.h>
#include <iostream>
#include <filesystem>

// taken from https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames#Common_programming_languages
/**/
int long2tilex(double lon, int z)
{
  return (int)(floor((lon + 180.0) / 360.0 * (1 << z)));
}

int lat2tiley(double lat, int z)
{
  double latrad = lat * M_PI / 180.0;
  return (int)(floor((1.0 - asinh(tan(latrad)) / M_PI) / 2.0 * (1 << z)));
}

double tilex2long(int x, int z)
{
  return x / (double)(1 << z) * 360.0 - 180;
}

double tiley2lat(int y, int z)
{
  double n = M_PI - 2.0 * M_PI * y / (double)(1 << z);
  return 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
}

double tilex2long(double x, int z)
{
  return x / (double)(1 << z) * 360.0 - 180;
}
double tiley2lat(double y, int z)
{
  double n = M_PI - 2.0 * M_PI * y / (double)(1 << z);
  return 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
}

// TODO: Implement the above functions to use transformations and manifest for wmts server

int main(int argc, char *argv[])
{
  GDALAllRegister();

  GDALDataset *poSrcDataset;
  poSrcDataset = (GDALDataset *)GDALOpen(argv[1], GA_ReadOnly);
  if (poSrcDataset == NULL)
  {
    std::cerr << "Error: cannot open file " << argv[1] << std::endl;
    exit(1);
  }

  int nXSize = poSrcDataset->GetRasterXSize();
  int nYSize = poSrcDataset->GetRasterYSize();
  int nBands = poSrcDataset->GetRasterCount();
  GDALDataType eType = poSrcDataset->GetRasterBand(1)->GetRasterDataType();
  std::cout << "Size is " << nXSize << " x " << nYSize << " x " << nBands << " datatype " << GDALGetDataTypeName(eType) << std::endl;

  // TODO: Intiger handling
  if (nBands != 1 || eType != GDT_Float32)
  {
    std::cerr << "[Error]: unsupported format, convert to float32 with one band" << std::endl;
    exit(1);
  }

  float *gridBuffer = new float[nXSize * nYSize];

  std::cout << "[]     Reading data..." << std::endl;
  if (poSrcDataset->GetRasterBand(1)->RasterIO(GF_Read, 0, 0, nXSize, nYSize, gridBuffer, nXSize, nYSize, GDT_Float32, 0, 0) != CE_None)
  {
    std::cout << "ERROR: cannot read data" << std::endl;
    exit(1);
  }
  std::cout << "[ ok ] read data successfully" << std::endl;

  std::cout << "[]     listing required tiles..." << std::endl;
  int k_tile_size = 256;

  int k_Zoom = 14;

  // find dataset geolocation
  double adfGeoTransform[6];
  if (poSrcDataset->GetGeoTransform(adfGeoTransform) != CE_None)
  {
    std::cerr << "Error: cannot get geotransform" << std::endl;
    exit(1);
  }
  std::cout << "[ GeoTransform ] " << adfGeoTransform[0] << ", " << adfGeoTransform[1] << ", " << adfGeoTransform[2] << ", " << adfGeoTransform[3] << ", " << adfGeoTransform[4] << ", " << adfGeoTransform[5] << std::endl;
  std::cout << long2tilex(adfGeoTransform[0], k_Zoom) << " " << lat2tiley((adfGeoTransform[3]), k_Zoom) << std::endl;

  // find the "root" tile, which contains all possible tiles
  int min_tile_x = long2tilex(adfGeoTransform[0], k_Zoom);
  int min_tile_y = lat2tiley((adfGeoTransform[3]), k_Zoom);
  int max_tile_x = long2tilex(adfGeoTransform[0] + adfGeoTransform[1] * nXSize, k_Zoom);
  int max_tile_y = lat2tiley((adfGeoTransform[3] + adfGeoTransform[5] * nYSize), k_Zoom);

  // check if min_y / max_y need to be swapped
  if (min_tile_y > max_tile_y)
  {
    int tmp = min_tile_y;
    min_tile_y = max_tile_y;
    max_tile_y = tmp;
  }

  //?: XYZ filenames, not tms

  GDALDriver *poDriverMEM = GetGDALDriverManager()->GetDriverByName("MEM");
  GDALDriver *poDriverHF = GetGDALDriverManager()->GetDriverByName("HF2");
  if (poDriverMEM == NULL || poDriverHF == NULL)
  {
    std::cerr << "Error: cannot get driver" << std::endl;
    exit(1);
  }

  // create directory structure
  std::filesystem::create_directory("./tileset");
  std::filesystem::create_directory("./tileset/" + std::to_string(k_Zoom));

  // creation optiosn for all tiles
  char **papszOptions = NULL;
  papszOptions = CSLSetNameValue(papszOptions, "COMPRESS", "YES");
  papszOptions = CSLSetNameValue(papszOptions, "VERTICAL_PRECISION", "0.0001");

  double tileOffsetX = tilex2long(long2tilex(adfGeoTransform[0], k_Zoom), k_Zoom) - adfGeoTransform[0];
  double tileOffsetY = tiley2lat(lat2tiley((adfGeoTransform[3]), k_Zoom), k_Zoom) - adfGeoTransform[3];

  std::cout << "Tile offset: " << tileOffsetX << " " << tileOffsetY << std::endl;
  // exit(0);

  int txcount = max_tile_x - min_tile_x + 1;
  int tycount = max_tile_y - min_tile_y + 1;

  for (int tile_x = 0; tile_x <= txcount; tile_x++)
  {
    std::filesystem::create_directory("./tileset/" + std::to_string(k_Zoom) + "/" + std::to_string(tile_x + min_tile_x));

    for (int tile_y = 0; tile_y <= tycount; tile_y++)
    {
      float *tileBuffer = new float[k_tile_size * k_tile_size];
      std::fill(tileBuffer, tileBuffer + k_tile_size * k_tile_size, 9999.0f);
      // std::cout << "Tile " << tile_x << " " << tile_y << std::endl;

      // find tile bounds
      double tile_x_lon = tilex2long(tile_x + min_tile_x, k_Zoom);
      double tile_y_lat = tiley2lat(tile_y + min_tile_y, k_Zoom);
      double tile_x_lon_end = tilex2long(tile_x + min_tile_x + 1, k_Zoom);
      double tile_y_lat_end = tiley2lat(tile_y + min_tile_y + 1, k_Zoom);

      double pixels_to_read_x = abs((tile_x_lon_end - tile_x_lon) / adfGeoTransform[1]);
      double pixels_to_read_y = abs((tile_y_lat - tile_y_lat_end) / adfGeoTransform[5]);

      // std::cout << "pixels to read: " << pixels_to_read_x << " " << pixels_to_read_y << std::endl;

      // find transformation betwwen tile pixel coordinates and dataset pixel coordinates
      double tile_x_offset = (tile_x_lon - adfGeoTransform[0]) / adfGeoTransform[1];
      double tile_y_offset = (tile_y_lat - adfGeoTransform[3]) / adfGeoTransform[5];

      int x1 = static_cast<int>(tile_x_offset);
      int y1 = static_cast<int>(tile_y_offset);

      int x2 = static_cast<int>(tile_x_offset + pixels_to_read_x);
      int y2 = static_cast<int>(tile_y_offset + pixels_to_read_y);

      for (int ix = 0; ix < k_tile_size; ix++)
      {
        for (int iy = 0; iy < k_tile_size; iy++)
        {
          double x = x1 + double(ix) / double(k_tile_size) * double(x2 - x1);
          double y = y1 + double(iy) / double(k_tile_size) * double(y2 - y1);

          if (x < 0 || y < 0 || x + 1 >= nXSize || y + 1 >= nYSize)
          {
            continue;
          }

          int grid_index_ul = (int)y * nXSize + (int)x;
          int grid_index_ur = (int)y * nXSize + (int)x + 1;
          int grid_index_ll = (int)y * nXSize + (int)x + nXSize;
          int grid_index_lr = (int)y * nXSize + (int)x + nXSize + 1;

          int tile_index = iy * k_tile_size + ix;

          if (gridBuffer[grid_index_ul] == -9999.0f || gridBuffer[grid_index_ur] == -9999.0f || gridBuffer[grid_index_ll] == -9999.0f || gridBuffer[grid_index_lr] == -9999.0f)
            continue;

          // average
          // tileBuffer[tile_index] = (gridBuffer[grid_index_ul] + gridBuffer[grid_index_ur] + gridBuffer[grid_index_ll] + gridBuffer[grid_index_lr]) / 4.0;

          // bilinear interpolation
          double x1_bi = x - (int)x;
          double y1_bi = y - (int)y;
          double x2_bi = 1 - x1_bi;
          double y2_bi = 1 - y1_bi;

          tileBuffer[tile_index] = gridBuffer[grid_index_ul] * x2_bi * y2_bi + gridBuffer[grid_index_ur] * x1_bi * y2_bi + gridBuffer[grid_index_ll] * x2_bi * y1_bi + gridBuffer[grid_index_lr] * x1_bi * y1_bi;
        }
      }

      GDALDataset *poDstDatasetTmp;
      char filename[256];
      // snprintf(filename, 256, "/vsimem/tileset/%d/%d/%d.hf2.gtiff", k_Zoom, tile_x + min_tile_x, tile_y + min_tile_y);
      poDstDatasetTmp = poDriverMEM->Create("", k_tile_size, k_tile_size, 1, GDT_Float32, NULL);
      if (poDstDatasetTmp == NULL)
      {
        std::cerr << "Error: cannot create dataset" << std::endl;
        exit(1);
      }
      double *adfTileGeoTransform = new double[6];
      adfTileGeoTransform[0] = tilex2long(tile_x + min_tile_x, k_Zoom);
      adfTileGeoTransform[1] = adfGeoTransform[1] * double(pixels_to_read_x) / double(k_tile_size);
      adfTileGeoTransform[2] = 0;
      adfTileGeoTransform[3] = tiley2lat(tile_y + min_tile_y, k_Zoom);
      adfTileGeoTransform[4] = 0;
      adfTileGeoTransform[5] = adfGeoTransform[5] * double(pixels_to_read_y) / double(k_tile_size);
      poDstDatasetTmp->SetGeoTransform(adfTileGeoTransform);

      if (poDstDatasetTmp->GetRasterBand(1)->RasterIO(GF_Write, 0, 0, k_tile_size, k_tile_size, tileBuffer, k_tile_size, k_tile_size, GDT_Float32, 0, 0) != CE_None)
      {
        std::cerr << "Error: cannot write data" << std::endl;
        exit(1);
      }

      poDstDatasetTmp->SetProjection(poSrcDataset->GetProjectionRef());

      snprintf(filename, 256, "./tileset/%d/%d/%d.hfz", k_Zoom, tile_x + min_tile_x, tile_y + min_tile_y);

      GDALDataset *poDstDataset;
      poDstDataset = poDriverHF->CreateCopy(filename, poDstDatasetTmp, FALSE, papszOptions, NULL, NULL);

      GDALClose(poDstDatasetTmp);
      GDALClose(poDstDataset);
      delete[] adfTileGeoTransform;
      delete[] tileBuffer;
      // delete[] readBuffer;
    }
  }

  std::cout << "[]     Done" << std::endl;

  GDALDestroy();
  return 0;
}