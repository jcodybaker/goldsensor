cmake_minimum_required(VERSION 3.16.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(goldsensor4)
list(APPEND EXTRA_COMPONENT_DIRS mdns)