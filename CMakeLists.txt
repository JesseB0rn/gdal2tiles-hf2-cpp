cmake_minimum_required(VERSION 3.10)

# Set the project name
project(tif2tileproj)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Enable testing
enable_testing()

# Find GDAL package
find_package(GDAL REQUIRED)
# Include GDAL headers
include_directories(${GDAL_INCLUDE_DIRS})

find_package(Threads REQUIRED)
# find_package(absl REQUIRED)

# Add GoogleTest
include(FetchContent)
FetchContent_Declare(
	googletest
	URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

# Add the executable
add_executable(tif2tile src/tif2tile.cpp)

add_executable(hfzwmtstiler src/hfzwmtstiler.cpp src/tile2lnglat.h)

add_executable(tiler25 
src/tiler_primitives.cpp
src/tiler.cpp)
# Link GDAL library
target_link_libraries(tif2tile ${GDAL_LIBRARIES})
target_link_libraries(hfzwmtstiler ${GDAL_LIBRARIES})
target_link_libraries(tiler25 ${GDAL_LIBRARIES})

# Add test executable (example, will add test file next)
add_executable(
	primitives_test
	tests/tiler_tests.cpp
  src/tiler_primitives.cpp
)
target_link_libraries(
	primitives_test
	gtest_main
)
gtest_discover_tests(primitives_test)


