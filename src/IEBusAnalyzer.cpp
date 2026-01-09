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

#include "IEBusAnalyzer.hpp"

#include <AnalyzerChannelData.h>

#include <memory>

#include "IEBusAnalyzerSettings.hpp"

namespace {

auto constexpr PARITY_ERROR = 1 << 0;
auto constexpr NAK = 1 << 1;

auto constexpr CONTROL = 100;
auto constexpr LENGTH = 101;
auto constexpr DATA = 102;
auto constexpr HEADER = 103;

} // namespace

IEBusAnalyzer::IEBusAnalyzer() : Analyzer2(), m_settings(), m_simulationInitialized(false) {
  m_results = std::make_unique<IEBusAnalyzerResults>(this, &m_settings);

  SetAnalyzerSettings(&m_settings);

  m_toleranceStart = m_settings.getStartBitWidth() / 10;
  m_toleranceBit = m_settings.getBitWidth() / 10;
  m_zeroBitLen = static_cast<U32>((static_cast<double>(m_settings.getBitWidth()) * 0.875));
  m_data = 0;
  m_flags = 0;

  // mEndOfStopBitOffset = 0;
  // mSampleRateHz = 0;
  // mSerial = 0;
}

IEBusAnalyzer::~IEBusAnalyzer() {
  KillThread();
}

auto IEBusAnalyzer::SetupResults() -> void {
  // SetupResults is called each time the analyzer is run. Because the same instance can be used for multiple runs, we need to clear the results each time.
  m_results = std::make_unique<IEBusAnalyzerResults>(this, &m_settings);

  SetAnalyzerResults(m_results.get());

  m_results->AddChannelBubblesWillAppearOn(m_settings.getInputChannel());
}

[[noreturn]] auto IEBusAnalyzer::WorkerThread() -> void {
  m_sampleRateHz = GetSampleRate();

  auto inputChannel = m_settings.getInputChannel();

  m_serial = GetAnalyzerChannelData(inputChannel);

  U8 data_mask = 1 << 7;

  for (;;) {
    if (m_serial->GetBitState() == BIT_LOW) {
      m_serial->AdvanceToNextEdge();
    }

    m_data = 0;
    int length = 0;

    // flag for starting bit
    bool found_start = false;
    bool finished_field = false;
    int bit_counter = 0;
    m_startSampleNumberStart = m_serial->GetSampleNumber();

    // search for the starting bit
    while (!found_start) {
      // go to edge of what might be the starting bit and get the sample number
      m_serial->AdvanceToNextEdge();
      m_startSampleNumberFinish = m_serial->GetSampleNumber();

      // measure the sample and normilize the units
      m_measureWidth = (m_startSampleNumberFinish - m_startSampleNumberStart);

      // check to see if we have a starting bit
      // if so let us continue to collect data
      // if not we will reset the starting position
      if ((m_measureWidth > (m_settings.getStartBitWidth() - m_toleranceStart)) && (m_measureWidth < (m_settings.getStartBitWidth() + m_toleranceStart))) {
        m_data = 0xFFFF;
        found_start = true;
        m_results->AddMarker(m_startSampleNumberStart, AnalyzerResults::UpArrow, inputChannel);
        m_results->AddMarker(m_startSampleNumberFinish, AnalyzerResults::Start, inputChannel);
      } else {
        m_serial->AdvanceToNextEdge();
        m_startSampleNumberStart = m_serial->GetSampleNumber();
      }
    }
    update(m_startSampleNumberStart, m_data, 0, 0);

    // get the header
    m_startSampleNumberStart = m_serial->GetSampleNumber();
    m_results->AddMarker(m_serial->GetSampleNumber(), AnalyzerResults::Dot, inputChannel);
    m_serial->AdvanceToNextEdge();
    m_startSampleNumberFinish = m_serial->GetSampleNumber();
    m_measureWidth = (m_startSampleNumberFinish - m_startSampleNumberStart);
    if ((m_measureWidth > ((m_settings.getBitWidth() / 2) - m_toleranceBit)) && (m_measureWidth < ((m_settings.getBitWidth() / 2) + m_toleranceBit))) {
      m_results->AddMarker(m_serial->GetSampleNumber(), AnalyzerResults::One, inputChannel);
      m_data = 1;
    } else {
      m_results->AddMarker(m_serial->GetSampleNumber(), AnalyzerResults::Zero, inputChannel);
      m_data = 0;
    }
    update(m_startSampleNumberStart, m_data, HEADER, 0);

    // get the master address
    if (not getAddress(true)) {
      continue;
    }

    // get the slave address
    if (not getAddress(false)) {
      continue;
    }

    // get control
    getData(CONTROL);
    // get length
    length = getData(LENGTH);
    while (length > 0) {
      getData(DATA);
      length--;
    }
  }
}

auto IEBusAnalyzer::update(U64 startingSample, U64& data1, U8 type, U8 flags) -> void {
  Frame f;
  f.mData1 = U32(data1);
  f.mData2 = type;
  f.mFlags = flags;
  f.mStartingSampleInclusive = startingSample;
  f.mEndingSampleInclusive = m_serial->GetSampleNumber();

  m_results->AddFrame(f);
  m_results->CommitResults();

  ReportProgress(f.mEndingSampleInclusive);

  // move to next rising edge
  m_serial->AdvanceToNextEdge();
  data1 = 0;
}

auto IEBusAnalyzer::parity(U16 data) -> U8 {
  int parity = 0;
  for (int x = 0; x < 8; x++) {
    if (data & 1 << x)
      parity++;
  }
  if (parity % 2)
    return 1;
  else
    return 0;
}

auto IEBusAnalyzer::getAddress(bool master) -> bool {
  U16 mask = 1 << 11;
  m_data = 0;
  m_flags = 0;
  m_startSampleNumberStart = m_serial->GetSampleNumber();
  m_startBitNumberStart = m_startSampleNumberStart;

  auto inputChannel = m_settings.getInputChannel();

  int i = 0;
  while (i < 12) {
    m_results->AddMarker(m_startBitNumberStart, AnalyzerResults::Dot, inputChannel);
    m_serial->AdvanceToNextEdge();
    m_startSampleNumberFinish = m_serial->GetSampleNumber();
    m_measureWidth = (m_startSampleNumberFinish - m_startBitNumberStart);
    if ((m_measureWidth > (m_settings.getBitWidth() / 2 - m_toleranceBit)) && (m_measureWidth < ((m_settings.getBitWidth() / 2) + m_toleranceBit))) {
      m_results->AddMarker(m_serial->GetSampleNumber(), AnalyzerResults::One, inputChannel);
      m_data |= mask;
    } else if ((m_measureWidth > (m_zeroBitLen - m_toleranceBit)) && (m_measureWidth < (m_zeroBitLen + m_toleranceBit))) {
      m_results->AddMarker(m_serial->GetSampleNumber(), AnalyzerResults::Zero, inputChannel);
    } else {
      return false;
    }
    i++;
    if (i < 12) {
      m_serial->AdvanceToNextEdge();
      m_startBitNumberStart = m_serial->GetSampleNumber();
      mask = mask >> 1;
    }
    // get the parity
    else {
      m_serial->AdvanceToNextEdge();
      m_startBitNumberStart = m_serial->GetSampleNumber();
      m_results->AddMarker(m_serial->GetSampleNumber(), AnalyzerResults::Dot, inputChannel);
      m_serial->AdvanceToNextEdge();
      m_startSampleNumberFinish = m_serial->GetSampleNumber();
      m_measureWidth = (m_startSampleNumberFinish - m_startBitNumberStart);
      if (m_measureWidth > (m_settings.getBitWidth() / 2 - m_toleranceBit) && m_measureWidth < (m_settings.getBitWidth() / 2 + m_toleranceBit)) {
        m_results->AddMarker(m_serial->GetSampleNumber(), AnalyzerResults::One, inputChannel);
        m_flags = 1;
      } else if ((m_measureWidth > (m_zeroBitLen - m_toleranceBit)) && (m_measureWidth < (m_zeroBitLen + m_toleranceBit))) {
        m_results->AddMarker(m_serial->GetSampleNumber(), AnalyzerResults::Zero, inputChannel);
        m_flags = 0;
      } else
        return false;
      m_flags = m_flags & parity(static_cast<U16>(m_data));
    }
  }
  // if slave address we should expect a slave ack
  if (not master) {
    m_serial->AdvanceToNextEdge();
    m_startBitNumberStart = m_serial->GetSampleNumber();
    m_results->AddMarker(m_serial->GetSampleNumber(), AnalyzerResults::Dot, inputChannel);
    m_serial->AdvanceToNextEdge();
    m_startSampleNumberFinish = m_serial->GetSampleNumber();
    m_measureWidth = (m_startSampleNumberFinish - m_startBitNumberStart);
    if (m_measureWidth > (m_settings.getBitWidth() / 2 - m_toleranceBit) && m_measureWidth < (m_settings.getBitWidth() / 2 + m_toleranceBit)) {
      m_results->AddMarker(m_serial->GetSampleNumber(), AnalyzerResults::One, inputChannel);
      m_flags = NAK;
    } else if ((m_measureWidth > (m_zeroBitLen - m_toleranceBit)) && (m_measureWidth < (m_zeroBitLen + m_toleranceBit))) {
      m_results->AddMarker(m_serial->GetSampleNumber(), AnalyzerResults::Zero, inputChannel);
    } else
      return false;
  }
  update(m_startSampleNumberStart, m_data, master, 0 /*flags*/); // return flags for NACK
  return (m_flags != NAK);
}

auto IEBusAnalyzer::getData(U8 dataType) -> int {
  U8 numBits = 8;
  U16 mask = 1 << 7;
  if (dataType == CONTROL) {
    numBits = 4;
    mask = 1 << 3;
  }
  m_data = 0;
  m_flags = 0;
  m_startSampleNumberStart = m_serial->GetSampleNumber();
  m_startBitNumberStart = m_startSampleNumberStart;

  auto inputChannel = m_settings.getInputChannel();

  int i = 0;
  while (i < numBits) {
    m_results->AddMarker(m_startBitNumberStart, AnalyzerResults::Dot, inputChannel);
    m_serial->AdvanceToNextEdge();
    m_startSampleNumberFinish = m_serial->GetSampleNumber();
    m_measureWidth = (m_startSampleNumberFinish - m_startBitNumberStart);
    if (m_measureWidth > (m_settings.getBitWidth() / 2 - m_toleranceBit) && m_measureWidth < (m_settings.getBitWidth() / 2 + m_toleranceBit)) {
      m_results->AddMarker(m_serial->GetSampleNumber(), AnalyzerResults::One, inputChannel);
      m_data |= mask;
    } else {
      m_results->AddMarker(m_serial->GetSampleNumber(), AnalyzerResults::Zero, inputChannel);
    }
    i++;
    if (i < numBits) {
      m_serial->AdvanceToNextEdge();
      m_startBitNumberStart = m_serial->GetSampleNumber();
      mask = mask >> 1;
    }
    // get the parity
    else {
      m_serial->AdvanceToNextEdge();
      m_startBitNumberStart = m_serial->GetSampleNumber();
      m_results->AddMarker(m_serial->GetSampleNumber(), AnalyzerResults::Dot, inputChannel);
      m_serial->AdvanceToNextEdge();
      m_startSampleNumberFinish = m_serial->GetSampleNumber();
      m_measureWidth = (m_startSampleNumberFinish - m_startBitNumberStart);
      if (m_measureWidth > (m_settings.getBitWidth() / 2 - m_toleranceBit) && m_measureWidth < (m_settings.getBitWidth() / 2 + m_toleranceBit)) {
        m_results->AddMarker(m_serial->GetSampleNumber(), AnalyzerResults::One, inputChannel);
        // flags = 1;
      } else {
        m_results->AddMarker(m_serial->GetSampleNumber(), AnalyzerResults::Zero, inputChannel);
        // flags = 0;
      }
      // flags = flags & parity(data);
    }
  }

  // we should always expect a ACK
  m_serial->AdvanceToNextEdge();
  m_startBitNumberStart = m_serial->GetSampleNumber();
  m_results->AddMarker(m_serial->GetSampleNumber(), AnalyzerResults::Dot, inputChannel);
  m_serial->AdvanceToNextEdge();
  m_startSampleNumberFinish = m_serial->GetSampleNumber();
  m_measureWidth = (m_startSampleNumberFinish - m_startBitNumberStart);
  if (m_measureWidth > (m_settings.getBitWidth() / 2 - m_toleranceBit) && m_measureWidth < (m_settings.getBitWidth() / 2 + m_toleranceBit)) {
    m_results->AddMarker(m_serial->GetSampleNumber(), AnalyzerResults::One, inputChannel);
    m_flags = m_flags | 2;
  } else {
    m_results->AddMarker(m_serial->GetSampleNumber(), AnalyzerResults::Zero, inputChannel);
  }
  // data gets reset in update, so save a copy for returning
  U8 tempData = static_cast<U8>(m_data);
  update(m_startSampleNumberStart, m_data, dataType, m_flags);
  return tempData;
}

auto IEBusAnalyzer::GenerateSimulationData(U64 minimumSampleIndex, U32 sampleRate, SimulationChannelDescriptor** simulationChannels) -> U32 {
  if (not m_simulationInitialized) {
    m_simulationInitialized = true;

    mSimulationDataGenerator.Initialize(GetSimulationSampleRate(), &m_settings);
  }

  return mSimulationDataGenerator.GenerateSimulationData(minimumSampleIndex, sampleRate, simulationChannels);
}

auto IEBusAnalyzer::GetMinimumSampleRateHz() -> U32 {
  return m_settings.getStartBitWidth() * 4;
}

auto IEBusAnalyzer::GetAnalyzerName() const -> char const* {
  return "IEbus";
}

auto IEBusAnalyzer::NeedsRerun() -> bool {
  return false;
}

auto GetAnalyzerName() ->  char const* {
  return "IEbus";
}

auto CreateAnalyzer() -> Analyzer* {
  return new IEBusAnalyzer();
}

auto DestroyAnalyzer(Analyzer* analyzer) -> void {
  delete analyzer;
}
