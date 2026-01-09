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

#include <Analyzer.h>
#include <memory>

#include "IEBusAnalyzerResults.hpp"
#include "IEBusAnalyzerSettings.hpp"
#include "IEBusSimulationDataGenerator.hpp"

class ANALYZER_EXPORT IEBusAnalyzer : public Analyzer2 {
private:
  using ResultPtr = std::unique_ptr<IEBusAnalyzerResults>;

public:
  IEBusAnalyzer();
  ~IEBusAnalyzer() override;

public:
  auto SetupResults() -> void override;
  [[noreturn]] [[noreturn]] auto WorkerThread() -> void override;

public:
  [[nodiscard]] auto GenerateSimulationData(U64 minimumSampleIndex, U32 sampleRate, SimulationChannelDescriptor** simulationChannels) -> U32 override;
  [[nodiscard]] auto GetMinimumSampleRateHz() -> U32 override;

public:
  [[nodiscard]] auto GetAnalyzerName() const -> char const* override;
  [[nodiscard]] auto NeedsRerun() -> bool override;

private:
  // let's make this so we don't have DRY
  auto update(U64 startingSample, U64& data1, U8 type, U8 flags) -> void;
  // simple parity check :: returns 1 if even number of bits, returns 0 if odd number of bits
  auto parity(U16 data) -> U8;
  auto getAddress(bool master) -> bool;
  auto getData(U8 dataType) -> int;

private:
  ResultPtr m_results = nullptr;

private:
  IEBusAnalyzerSettings m_settings;
  AnalyzerChannelData* m_serial;

  IEBusSimulationDataGenerator mSimulationDataGenerator;
  bool m_simulationInitialized;

  // Serial analysis vars:
  U32 m_sampleRateHz;
  U32 m_startOfStopBitOffset;
  U32 m_endOfStopBitOffset;
  // tolerance for instability of readings
  U32 m_toleranceStart;
  U32 m_toleranceBit;
  U32 m_zeroBitLen;
  // measure width for each bit.
  U64 m_measureWidth;
  // to hold the start of the start bit
  U64 m_startSampleNumberStart;
  // to hold the finish of the start bit
  U64 m_startSampleNumberFinish;
  // to hold the start of the data bit
  U64 m_startBitNumberStart;
  // data output
  U64 m_data;
  // flags
  // bit 0 = parity error
  // bit 1 = NAK
  U8 m_flags;
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer();
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer(Analyzer* analyzer);
