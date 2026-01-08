cmake_minimum_required(VERSION 3.31.6)

set(SOURCES
        SimpleSerialAnalyzer.cpp
        SimpleSerialAnalyzer.h
        SimpleSerialAnalyzerResults.cpp
        SimpleSerialAnalyzerResults.h
        SimpleSerialAnalyzerSettings.cpp
        SimpleSerialAnalyzerSettings.h
        SimpleSerialSimulationDataGenerator.cpp
        SimpleSerialSimulationDataGenerator.h
)

add_analyzer_plugin(${PROJECT_NAME} SOURCES ${SOURCES})
