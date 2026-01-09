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

#include "IEBusSimulationDataGenerator.hpp"

#include <AnalyzerHelpers.h>

#include "IEBusAnalyzerSettings.hpp"

IEBusSimulationDataGenerator::IEBusSimulationDataGenerator() : m_serialText("My first analyzer, woo hoo!"), m_stringIndex(0), m_settings(nullptr), m_simulationSampleRateHz(30000) {
}

void IEBusSimulationDataGenerator::Initialize(U32 simulationSampleRate, IEBusAnalyzerSettings* settings) {
  m_simulationSampleRateHz = simulationSampleRate;
  m_settings = settings;

  auto inputChannel = m_settings->getInputChannel();

  m_serialSimulationData.SetChannel(inputChannel);
  m_serialSimulationData.SetSampleRate(simulationSampleRate);
  m_serialSimulationData.SetInitialBitState(BIT_HIGH);
}

U32 IEBusSimulationDataGenerator::GenerateSimulationData(U64 largestSampleRequested, U32 sampleRate, SimulationChannelDescriptor** simulationChannel) {
  auto const adjustedLargestSampleRequested = AnalyzerHelpers::AdjustSimulationTargetSample(largestSampleRequested, sampleRate, m_simulationSampleRateHz);

  while (m_serialSimulationData.GetCurrentSampleNumber() < adjustedLargestSampleRequested) {
    CreateSerialByte();
  }

  *simulationChannel = &m_serialSimulationData;
  return 1;
}

void IEBusSimulationDataGenerator::CreateSerialByte() {
  auto const samplesPerBit = m_simulationSampleRateHz; // / m_settings->mBitRate;

  auto const byte = m_serialText[m_stringIndex];
  m_stringIndex++;
  if (m_stringIndex == m_serialText.size())
    m_stringIndex = 0;

  // we're currenty high
  // let's move forward a little
  m_serialSimulationData.Advance(samplesPerBit * 10);

  m_serialSimulationData.Transition();           // low-going edge for start bit
  m_serialSimulationData.Advance(samplesPerBit); // add start bit time

  auto mask = 0x1 << 7;
  for (U32 i = 0; i < 8; i++) {
    if ((byte & mask) != 0) {
      m_serialSimulationData.TransitionIfNeeded(BIT_HIGH);
    } else {
      m_serialSimulationData.TransitionIfNeeded(BIT_LOW);
    }

    m_serialSimulationData.Advance(samplesPerBit);
    mask = mask >> 1;
  }

  m_serialSimulationData.TransitionIfNeeded(BIT_HIGH); // we need to end high

  // lets pad the end a bit for the stop bit:
  m_serialSimulationData.Advance(samplesPerBit);
}
