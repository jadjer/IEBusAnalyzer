cmake_minimum_required(VERSION 3.31.6)

project(IEBusAnalyzer)

add_definitions(-DLOGIC2)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(ExternalAnalyzerSDK)

include_directories(include)

add_subdirectory(src)
