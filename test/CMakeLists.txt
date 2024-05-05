#idf_component_register(SRC_DIRS "."
#                       INCLUDE_DIRS "."
#                       REQUIRES unity)
#


# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

# Add newly added components to one of these lines:
# 1. Add here if the component is compatible with IDF >= v4.3
set(EXTRA_COMPONENT_DIRS "../components" )

set(TEST_COMPONENTS "neopixel_display" CACHE STRING "List of components to test")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(test_neopix_tetris)
