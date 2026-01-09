cmake_minimum_required(VERSION 3.31.6)

set(SOURCES
        IEBusAnalyzer.cpp
        IEBusAnalyzerResults.cpp
        IEBusAnalyzerSettings.cpp
        IEBusSimulationDataGenerator.cpp
)

add_analyzer_plugin(${PROJECT_NAME} SOURCES ${SOURCES})
