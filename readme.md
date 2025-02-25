This is a C++ implementation of the gdal2tiles.py script. It is based on the gdal2tiles.py script from the GDAL project. The script is used to create a tile pyramid from a georeferenced image. The tiles can be used in a web map application. At present, this tool only outputs HF2 tiles. (the original script can only handle uint8, this script does float32)

## building

Prereqs: GDAL and CMake installed

```
git clone https://github.com/JesseB0rn/gdal2tiles-hf2-cpp.git
cd gdal2tiles-hf2-cpp
mkdir build
cd build
cmake ..
make -j4
```

## creating riskmaps:

1. create riskmap: riskprocessor from MA
2. warp to pesg:4325 (WGS84):: `gdalwarp -t_srs EPSG:4326 riskmap.tif riskmap_4326.tif -overwrite`
3. ./tif2tile /Users/jesseb0rn/Documents/repos/Maturaarbeit-AlgoSkitour/source/riskprocessor/riskmap_4326.tif
