#include <gdal.h>
#include <gdal_priv.h>
#include <iostream>
#include <filesystem>

#include "tile2lnglat.h"

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
  if (nBands != 1 || !(eType == GDT_UInt16 || eType == GDT_Int16 || eType == GDT_Float32 || eType == GDT_Float64 || eType == GDT_Byte || eType == GDT_UInt32 || eType == GDT_Int32))
  {
    std::cerr << "[Error]: unsupported format, convert to u/int16 with one band" << std::endl;
    exit(1);
  }
  void *gridBuffer = nullptr;

  if (eType == GDT_UInt16)
  {
    gridBuffer = new uint16_t[nXSize * nYSize];
  }
  else if (eType == GDT_Int16)
  {
    gridBuffer = new int16_t[nXSize * nYSize];
  }
  else if (eType == GDT_Float32)
  {
    gridBuffer = new float[nXSize * nYSize];
  }
  else if (eType == GDT_Float64)
  {
    gridBuffer = new double[nXSize * nYSize];
  }
  else if (eType == GDT_Byte)
  {
    gridBuffer = new uint8_t[nXSize * nYSize];
  }
  else if (eType == GDT_UInt32)
  {
    gridBuffer = new uint32_t[nXSize * nYSize];
  }
  else if (eType == GDT_Int32)
  {
    gridBuffer = new int32_t[nXSize * nYSize];
  }
  else
  {
    std::cerr << "[Error]: unsupported data type" << std::endl;
    exit(1);
  }
  // float *gridBuffer = new float[nXSize * nYSize];

  std::cout << "[]     Reading data..." << std::endl;
  if (poSrcDataset->GetRasterBand(1)->RasterIO(GF_Read, 0, 0, nXSize, nYSize, gridBuffer, nXSize, nYSize, eType, 0, 0) != CE_None)
  {
    std::cout << "ERROR: cannot read data" << std::endl;
    exit(1);
  }
  std::cout << "[ ok ] read data successfully" << std::endl;

  std::cout << "[]     listing required tiles..." << std::endl;
  int k_tile_size = 256;

  // find dataset geolocation
  double adfGeoTransform[6];
  if (poSrcDataset->GetGeoTransform(adfGeoTransform) != CE_None)
  {
    std::cerr << "Error: cannot get geotransform" << std::endl;
    exit(1);
  }
  std::cout << "[ GeoTransform ] " << adfGeoTransform[0] << ", " << adfGeoTransform[1] << ", " << adfGeoTransform[2] << ", " << adfGeoTransform[3] << ", " << adfGeoTransform[4] << ", " << adfGeoTransform[5] << std::endl;

  GDALDriver *poDriverMEM = GetGDALDriverManager()->GetDriverByName("MEM");
  GDALDriver *poDriverHF = GetGDALDriverManager()->GetDriverByName("HF2");
  if (poDriverMEM == NULL || poDriverHF == NULL)
  {
    std::cerr << "Error: cannot get driver" << std::endl;
    exit(1);
  }

  // create directory structure
  std::filesystem::create_directory("./tileset");

  // creation optiosn for all tiles
  char **papszOptions = NULL;
  papszOptions = CSLSetNameValue(papszOptions, "COMPRESS", "YES");
  papszOptions = CSLSetNameValue(papszOptions, "VERTICAL_PRECISION", "1");

  WMTSTileMatrixInfoBlock tile_matrix_info;
  tile_matrix_info.scaleDenominator = 0.00028;
  tile_matrix_info.topLeftCorner[0] = adfGeoTransform[0];
  tile_matrix_info.topLeftCorner[1] = adfGeoTransform[3];
  tile_matrix_info.matrixWidth = nXSize / k_tile_size + 1;
  tile_matrix_info.matrixHeight = nYSize / k_tile_size + 1;

  for (int tile_x = 0; tile_x <= tile_matrix_info.matrixWidth; tile_x++)
  {
    std::filesystem::create_directory("./tileset/" + std::to_string(tile_x));

    for (int tile_y = 0; tile_y <= tile_matrix_info.matrixHeight; tile_y++)
    {
      void *tileBuffer = nullptr;
      if (eType == GDT_UInt16)
      {
        tileBuffer = new uint16_t[k_tile_size * k_tile_size];
      }
      else if (eType == GDT_Int16)
      {
        tileBuffer = new int16_t[k_tile_size * k_tile_size];
      }
      else if (eType == GDT_Float32)
      {
        tileBuffer = new float[k_tile_size * k_tile_size];
      }
      else if (eType == GDT_Float64)
      {
        tileBuffer = new double[k_tile_size * k_tile_size];
      }
      else if (eType == GDT_Byte)
      {
        tileBuffer = new uint8_t[k_tile_size * k_tile_size];
      }
      else if (eType == GDT_UInt32)
      {
        tileBuffer = new uint32_t[k_tile_size * k_tile_size];
      }
      else if (eType == GDT_Int32)
      {
        tileBuffer = new int32_t[k_tile_size * k_tile_size];
      }
      else
      {
        std::cerr << "[Error]: unsupported data type" << std::endl;
        exit(1);
      }

      if (eType == GDT_UInt16)
      {
        std::fill(reinterpret_cast<uint16_t *>(tileBuffer), reinterpret_cast<uint16_t *>(tileBuffer) + k_tile_size * k_tile_size, static_cast<uint16_t>(9999));
      }
      else if (eType == GDT_Int16)
      {
        std::fill(reinterpret_cast<int16_t *>(tileBuffer), reinterpret_cast<int16_t *>(tileBuffer) + k_tile_size * k_tile_size, static_cast<int16_t>(9999));
      }
      else if (eType == GDT_Float32)
      {
        std::fill(reinterpret_cast<float *>(tileBuffer), reinterpret_cast<float *>(tileBuffer) + k_tile_size * k_tile_size, 9999.0f);
      }
      else if (eType == GDT_Float64)
      {
        std::fill(reinterpret_cast<double *>(tileBuffer), reinterpret_cast<double *>(tileBuffer) + k_tile_size * k_tile_size, 9999.0);
      }
      else if (eType == GDT_Byte)
      {
        std::fill(reinterpret_cast<uint8_t *>(tileBuffer), reinterpret_cast<uint8_t *>(tileBuffer) + k_tile_size * k_tile_size, static_cast<uint8_t>(255)); // Assuming 255 as a placeholder for "no data"
      }
      else if (eType == GDT_UInt32)
      {
        std::fill(reinterpret_cast<uint32_t *>(tileBuffer), reinterpret_cast<uint32_t *>(tileBuffer) + k_tile_size * k_tile_size, static_cast<uint32_t>(9999));
      }
      else if (eType == GDT_Int32)
      {
        std::fill(reinterpret_cast<int32_t *>(tileBuffer), reinterpret_cast<int32_t *>(tileBuffer) + k_tile_size * k_tile_size, static_cast<int32_t>(9999));
      }
      else
      {
        std::cerr << "[Error]: unsupported data type for filling tile buffer" << std::endl;
        exit(1);
      }

      for (int ix = 0; ix < k_tile_size; ix++)
      {
        for (int iy = 0; iy < k_tile_size; iy++)
        {
          int grid_index = (k_tile_size * tile_y + iy) * nXSize + (k_tile_size * tile_x + ix);
          int tile_index = iy * k_tile_size + ix;

          // Bounds check to prevent out-of-bounds access
          if (grid_index >= nXSize * nYSize || grid_index < 0)
          {
            // std::cerr << "Error: grid index out of bounds" << std::endl;
            continue;
          }

          if (tile_index >= k_tile_size * k_tile_size || tile_index < 0)
          {
            // std::cerr << "Error: tile index out of bounds" << std::endl;
            continue;
          }

          // Handle "no data" values
          if (eType == GDT_UInt16)
          {
            if (reinterpret_cast<uint16_t *>(gridBuffer)[grid_index] == static_cast<uint16_t>(-9999))
              continue;
          }
          else if (eType == GDT_Int16)
          {
            if (reinterpret_cast<int16_t *>(gridBuffer)[grid_index] == static_cast<int16_t>(-9999))
              continue;
          }
          else if (eType == GDT_Float32)
          {
            if (reinterpret_cast<float *>(gridBuffer)[grid_index] == -9999.0f)
              continue;
          }
          else if (eType == GDT_Float64)
          {
            if (reinterpret_cast<double *>(gridBuffer)[grid_index] == -9999.0)
              continue;
          }
          else if (eType == GDT_Byte)
          {
            if (reinterpret_cast<uint8_t *>(gridBuffer)[grid_index] == static_cast<uint8_t>(-9999))
              continue;
          }
          else if (eType == GDT_UInt32)
          {
            if (reinterpret_cast<uint32_t *>(gridBuffer)[grid_index] == static_cast<uint32_t>(-9999))
              continue;
          }
          else if (eType == GDT_Int32)
          {
            if (reinterpret_cast<int32_t *>(gridBuffer)[grid_index] == static_cast<int32_t>(-9999))
              continue;
          }

          // Copy data
          if (eType == GDT_UInt16)
          {
            reinterpret_cast<uint16_t *>(tileBuffer)[tile_index] = reinterpret_cast<uint16_t *>(gridBuffer)[grid_index];
          }
          else if (eType == GDT_Int16)
          {
            reinterpret_cast<int16_t *>(tileBuffer)[tile_index] = reinterpret_cast<int16_t *>(gridBuffer)[grid_index];
          }
          else if (eType == GDT_Float32)
          {
            reinterpret_cast<float *>(tileBuffer)[tile_index] = reinterpret_cast<float *>(gridBuffer)[grid_index];
          }
          else if (eType == GDT_Float64)
          {
            reinterpret_cast<double *>(tileBuffer)[tile_index] = reinterpret_cast<double *>(gridBuffer)[grid_index];
          }
          else if (eType == GDT_Byte)
          {
            reinterpret_cast<uint8_t *>(tileBuffer)[tile_index] = reinterpret_cast<uint8_t *>(gridBuffer)[grid_index];
          }
          else if (eType == GDT_UInt32)
          {
            reinterpret_cast<uint32_t *>(tileBuffer)[tile_index] = reinterpret_cast<uint32_t *>(gridBuffer)[grid_index];
          }
          else if (eType == GDT_Int32)
          {
            reinterpret_cast<int32_t *>(tileBuffer)[tile_index] = reinterpret_cast<int32_t *>(gridBuffer)[grid_index];
          }
        }
      }

      GDALDataset *poDstDatasetTmp;
      char filename[256];
      // snprintf(filename, 256, "/vsimem/tileset/%d/%d/%d.hf2.gtiff", k_Zoom, tile_x + min_tile_x, tile_y + min_tile_y);
      poDstDatasetTmp = poDriverMEM->Create("", k_tile_size, k_tile_size, 1, eType, NULL);
      if (poDstDatasetTmp == NULL)
      {
        std::cerr << "Error: cannot create dataset" << std::endl;
        exit(1);
      }
      double *adfTileGeoTransform = new double[6];
      adfTileGeoTransform[0] = adfGeoTransform[0] + adfGeoTransform[1] * double(k_tile_size * tile_x);
      adfTileGeoTransform[1] = adfGeoTransform[1];
      adfTileGeoTransform[2] = 0;
      adfTileGeoTransform[3] = adfGeoTransform[3] + adfGeoTransform[5] * double(k_tile_size * tile_y);
      adfTileGeoTransform[4] = 0;
      adfTileGeoTransform[5] = adfGeoTransform[5];

      poDstDatasetTmp->SetGeoTransform(adfTileGeoTransform);

      if (poDstDatasetTmp->GetRasterBand(1)->RasterIO(GF_Write, 0, 0, k_tile_size, k_tile_size, tileBuffer, k_tile_size, k_tile_size, eType, 0, 0) != CE_None)
      {
        std::cerr << "Error: cannot write data" << std::endl;
        exit(1);
      }

      poDstDatasetTmp->SetProjection(poSrcDataset->GetProjectionRef());

      snprintf(filename, 256, "./tileset/%d/%d.hfz", tile_x, tile_y);

      GDALDataset *poDstDataset;
      poDstDataset = poDriverHF->CreateCopy(filename, poDstDatasetTmp, FALSE, papszOptions, NULL, NULL);

      GDALClose(poDstDatasetTmp);
      GDALClose(poDstDataset);
      delete[] adfTileGeoTransform;
      delete[] reinterpret_cast<uint8_t *>(tileBuffer);

      // delete[] readBuffer;
    }
  }

  // Ensure proper cleanup of gridBuffer
  delete[] reinterpret_cast<uint8_t *>(gridBuffer);

  std::cout << "[]     Done" << std::endl;

  GDALDestroy();
  return 0;
}