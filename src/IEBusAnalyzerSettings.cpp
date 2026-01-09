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

#include "IEBusAnalyzerSettings.hpp"

#include <AnalyzerHelpers.h>

namespace {

auto constexpr START_BIT_TOTAL_US = 190;
auto constexpr START_BIT_HIGH_US  = 171;
auto constexpr START_BIT_LOW_US   = START_BIT_TOTAL_US - START_BIT_HIGH_US;

auto constexpr DATA_BIT_TOTAL_US  = 39;
auto constexpr DATA_BIT_0_HIGH_US = 33;
auto constexpr DATA_BIT_0_LOW_US  = DATA_BIT_TOTAL_US - DATA_BIT_0_HIGH_US;
auto constexpr DATA_BIT_1_HIGH_US = 20;
auto constexpr DATA_BIT_1_LOW_US  = DATA_BIT_TOTAL_US - DATA_BIT_1_HIGH_US;

} // namespace

IEBusAnalyzerSettings::IEBusAnalyzerSettings() : m_bitWidth(START_BIT_HIGH_US), m_startBitWidth(DATA_BIT_TOTAL_US), m_inputChannel(UNDEFINED_CHANNEL) {
  m_bitWidthInterface.SetTitleAndTooltip("Bit Width (uS)", "Specify the bit width in uS");
  m_bitWidthInterface.SetMax(6000000);
  m_bitWidthInterface.SetMin(1);
  m_bitWidthInterface.SetInteger(m_bitWidth);

  m_inputChannelInterface.SetTitleAndTooltip("Receive Channel", "Slave Receive Channel");
  m_inputChannelInterface.SetChannel(m_inputChannel);

  m_startBitWidthInterface.SetTitleAndTooltip("Start Bit Width (uS)", "Specify the start bit width in uS");
  m_startBitWidthInterface.SetMax(6000000);
  m_startBitWidthInterface.SetMin(1);
  m_startBitWidthInterface.SetInteger(m_startBitWidth);

  AddInterface(&m_bitWidthInterface);
  AddInterface(&m_inputChannelInterface);
  AddInterface(&m_startBitWidthInterface);

  AddExportOption(0, "Export as text/csv file");
  AddExportExtension(0, "text", "txt");
  AddExportExtension(0, "csv", "csv");

  ClearChannels();
  AddChannel(m_inputChannel, "IEbus", false);
}

auto IEBusAnalyzerSettings::getBitWidth() const -> int {
  return m_bitWidth;
}

auto IEBusAnalyzerSettings::getInputChannel() const -> Channel {
  return m_inputChannel;
}

auto IEBusAnalyzerSettings::getStartBitWidth() const -> int {
  return m_startBitWidth;
}

auto IEBusAnalyzerSettings::SetSettingsFromInterfaces() -> bool {
  m_bitWidth = m_bitWidthInterface.GetInteger();
  m_inputChannel = m_inputChannelInterface.GetChannel();
  m_startBitWidth = m_startBitWidthInterface.GetInteger();

  ClearChannels();
  AddChannel(m_inputChannel, "IEbus", true);

  return true;
}

auto IEBusAnalyzerSettings::LoadSettings(const char* settings) -> void {
  SimpleArchive text_archive;
  text_archive.SetString(settings);

  text_archive >> m_bitWidth;
  text_archive >> m_inputChannel;
  text_archive >> m_startBitWidth;

  ClearChannels();
  AddChannel(m_inputChannel, "IEbus", true);

  UpdateInterfacesFromSettings();
}

auto IEBusAnalyzerSettings::SaveSettings() -> char const* {
  SimpleArchive text_archive;

  text_archive >> m_bitWidth;
  text_archive << m_inputChannel;
  text_archive << m_startBitWidth;

  return SetReturnString(text_archive.GetString());
}

auto IEBusAnalyzerSettings::UpdateInterfacesFromSettings() -> void {
  m_bitWidth = m_bitWidthInterface.GetInteger();

  m_inputChannelInterface.SetChannel(m_inputChannel);
  m_startBitWidthInterface.SetInteger(m_startBitWidth);
}
