// Copyright 2026 Pavel Suprunov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <SimulationChannelDescriptor.h>
#include <string>

class IEBusAnalyzerSettings;

class IEBusSimulationDataGenerator {
public:
  IEBusSimulationDataGenerator();

public:
  auto Initialize(U32 simulationSampleRate, IEBusAnalyzerSettings* settings) -> void;
  auto GenerateSimulationData(U64 largestSampleRequested, U32 sampleRate, SimulationChannelDescriptor** simulationChannel) -> U32;

private:
  void CreateSerialByte();

private:
  IEBusAnalyzerSettings* m_settings = nullptr;

private:
  U32 m_stringIndex;
  std::string m_serialText;
  U32 m_simulationSampleRateHz;
  SimulationChannelDescriptor m_serialSimulationData;
};
