#ifndef TILE2LNGLAT
#define TILE2LNGLAT

#include <tuple>

typedef struct
{
  double scaleDenominator;
  double topLeftCorner[2];
  int matrixWidth;
  int matrixHeight;
} WMTSTileMatrixInfoBlock;

class WMTSLngLatConversion
{
public:
  static std::pair<int, int> crs_to_tile_coords(double x, double y, WMTSTileMatrixInfoBlock &tile_matrix_info, double pixel_size = 0.00028)
  {
    double scale_denominator = tile_matrix_info.scaleDenominator;
    double top_left_x = tile_matrix_info.topLeftCorner[0];
    double top_left_y = tile_matrix_info.topLeftCorner[1];
    int tile_width = tile_matrix_info.matrixWidth;
    int tile_height = tile_matrix_info.matrixHeight;

    return crs_to_tile_coords(x, y, top_left_x, top_left_y, scale_denominator, tile_width, tile_height, pixel_size);
  }
  static std::pair<int, int> crs_to_tile_coords(double x, double y, double top_left_x, double top_left_y, double scale_denominator, int tile_width, int tile_height, double pixel_size = 0.00028)
  {
    double resolution = scale_denominator * pixel_size;
    double tile_width_crs = tile_width * resolution;
    double tile_height_crs = tile_height * resolution;

    int tile_x = static_cast<int>((x - top_left_x) / tile_width_crs);
    int tile_y = static_cast<int>((top_left_y - y) / tile_height_crs);

    return {tile_x, tile_y};
  }
  static std::tuple<double, double, double, double> tile_bounds(int x, int y, WMTSTileMatrixInfoBlock &tile_matrix_info, double pixel_size = 0.00028)
  {
    double scale_denominator = tile_matrix_info.scaleDenominator;
    double top_left_x = tile_matrix_info.topLeftCorner[0];
    double top_left_y = tile_matrix_info.topLeftCorner[1];
    int tile_width = tile_matrix_info.matrixWidth;
    int tile_height = tile_matrix_info.matrixHeight;

    return tile_bounds(x, y, top_left_x, top_left_y, scale_denominator, tile_width, tile_height, pixel_size);
  }
  static std::tuple<double, double, double, double> tile_bounds(int x, int y, double top_left_x, double top_left_y, double scale_denominator, int tile_width, int tile_height, double pixel_size = 0.00028)
  {
    double resolution = scale_denominator * pixel_size;
    double tile_width_crs = tile_width * resolution;
    double tile_height_crs = tile_height * resolution;

    double minx = top_left_x + x * tile_width_crs;
    double maxx = minx + tile_width_crs;

    double maxy = top_left_y - y * tile_height_crs;
    double miny = maxy - tile_height_crs;

    return {minx, miny, maxx, maxy};
  }
};

#endif