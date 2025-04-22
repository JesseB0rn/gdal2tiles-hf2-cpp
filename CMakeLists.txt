cmake_minimum_required(VERSION 3.10)

# Set the project name
project(tif2tileproj)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find GDAL package
find_package(GDAL REQUIRED)
# Include GDAL headers
include_directories(${GDAL_INCLUDE_DIRS})

find_package(Threads REQUIRED)
find_package(absl REQUIRED)

# Add the executable
add_executable(tif2tile src/tif2tile.cpp)

add_executable(hfzwmtstiler src/hfzwmtstiler.cpp src/tile2lnglat.h)
# Link GDAL library
target_link_libraries(tif2tile ${GDAL_LIBRARIES})
target_link_libraries(hfzwmtstiler ${GDAL_LIBRARIES})


