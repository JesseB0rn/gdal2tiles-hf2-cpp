[![CMake build & Test](https://github.com/JesseB0rn/gdal2tiles-hf2-cpp/actions/workflows/cmake-single-platform.yml/badge.svg)](https://github.com/JesseB0rn/gdal2tiles-hf2-cpp/actions/workflows/cmake-single-platform.yml)

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
