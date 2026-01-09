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

#include "IEBusAnalyzerResults.hpp"

#include <AnalyzerHelpers.h>
#include <array>
#include <fstream>
#include <iostream>

#include "IEBusAnalyzer.hpp"
#include "IEBusAnalyzerSettings.hpp"

namespace {

auto constexpr PARITY_ERROR = 1 << 0;
auto constexpr NAK = 1 << 1;

auto constexpr CONTROL = 100;
auto constexpr LENGTH = 101;
auto constexpr DATA = 102;
auto constexpr HEADER = 103;

auto constexpr NUMBER_STRINGS = 128;

// this is the control bit functions
// this likely needs to move to results
#define READSLAVESTATUS 0x0
#define READANDLOCK 0x3
#define READLOCKLOWER 0x4
#define READLOCKUPPER 0x5
#define READANDUNLOCK 0x6
#define READDATA 0x7
#define WRITEANDLOCKCOMMAND 0xA
#define WRITEANDLOCKDATA 0xB
// #define WRITEANDLOCKCOMMAND 	0xE
#define WRITEDATA 0xF

} // namespace

IEBusAnalyzerResults::IEBusAnalyzerResults(IEBusAnalyzer* analyzer, IEBusAnalyzerSettings* settings) : AnalyzerResults(), m_settings(settings), m_analyzer(analyzer) {
}

auto IEBusAnalyzerResults::GenerateBubbleText(U64 frameIndex, Channel& channel, DisplayBase displayBase) -> void {
  ClearResultStrings();

  Frame frame = GetFrame(frameIndex);

  auto numberStrings = std::array<char, NUMBER_STRINGS>();

  AnalyzerHelpers::GetNumberString(frame.mData1, displayBase, 8, numberStrings.data(), NUMBER_STRINGS);

  if (frame.mFlags) {
    if (frame.mFlags & NAK) {
      AddResultString("NAK");
    }
  } else {
    AddResultString(numberStrings.data());
  }
}

auto IEBusAnalyzerResults::GenerateExportFile(const char* file, DisplayBase display_base, U32 export_type_user_id) -> void {
  std::ofstream fileStream(file, std::ios::out);

  auto const triggerSample = m_analyzer->GetTriggerSample();
  auto const sampleRate = m_analyzer->GetSampleRate();

  fileStream << "Time [s],Value" << std::endl;

  auto const numFrames = GetNumFrames();

  for (U32 i = 0; i < numFrames; i++) {
    Frame frame = GetFrame(i);

    char time_str[128];
    AnalyzerHelpers::GetTimeString(frame.mStartingSampleInclusive, triggerSample, sampleRate, time_str, 128);

    if (frame.mFlags) {
      if (frame.mFlags & NAK) {
        fileStream << time_str << "," << "NAK" << std::endl;
      }
    } else if (frame.mData1 == 0xFFFF) {
      fileStream << std::endl << "=======================================" << std::endl;
      fileStream << time_str << "," << " START" << std::endl;
    } else {
      char number_str[128];
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 128);
      if (frame.mData2 == CONTROL)
        fileStream << time_str << ", Control: " << number_str << std::endl;
      else if (frame.mData2 == LENGTH) {
        fileStream << time_str << ", Frame Length: " << number_str << std::endl;
        fileStream << time_str << ", DATA: ";
      } else if (frame.mData2 == DATA)
        fileStream << "," << number_str;
      else if (frame.mData2 == HEADER)
        fileStream << time_str << ", HEADER: " << number_str << std::endl;
      else if (frame.mData2 == 0)
        fileStream << time_str << ", SLAVE ADDRESS: " << number_str << std::endl;
      else
        fileStream << time_str << ", MASTER ADDRESS" << number_str << std::endl;
    }

    if (UpdateExportProgressAndCheckForCancel(i, numFrames)) {
      fileStream.close();
      return;
    }
  }

  fileStream << std::endl << std::endl << "=======================================" << std::endl;
  fileStream << " RAW DATA" << std::endl;

  for (U32 i = 0; i < numFrames; i++) {
    Frame frame = GetFrame(i);

    char time_str[128];
    AnalyzerHelpers::GetTimeString(frame.mStartingSampleInclusive, triggerSample, sampleRate, time_str, 128);
    char number_str[128];
    AnalyzerHelpers::GetNumberString(frame.mData1, Hexadecimal, 8, number_str, 128);
    fileStream << number_str << std::endl;

    if (UpdateExportProgressAndCheckForCancel(i, numFrames)) {
      fileStream.close();
      return;
    }
  }

  fileStream.close();
}

auto IEBusAnalyzerResults::GenerateFrameTabularText(U64 frame_index, DisplayBase display_base) -> void {
#ifdef SUPPORTS_PROTOCOL_SEARCH
  Frame frame = GetFrame(frame_index);
  ClearResultStrings();

  char number_str[128];
  AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 128);
  AddResultString(number_str);
#endif
}

auto IEBusAnalyzerResults::GeneratePacketTabularText(U64 packet_id, DisplayBase display_base) -> void {
  // not supported
}

auto IEBusAnalyzerResults::GenerateTransactionTabularText(U64 transaction_id, DisplayBase display_base) -> void {
  // not supported
}