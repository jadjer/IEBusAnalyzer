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

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class IEBusAnalyzerSettings : public AnalyzerSettings {
public:
  IEBusAnalyzerSettings();
  ~IEBusAnalyzerSettings() override = default;

public:
  [[nodiscard]] auto getBitWidth() const -> int;
  [[nodiscard]] auto getInputChannel() const -> Channel;
  [[nodiscard]] auto getStartBitWidth() const -> int;

public:
  auto SetSettingsFromInterfaces() -> bool override;
  auto LoadSettings(char const* settings) -> void override;
  auto SaveSettings() -> char const* override;

private:
  auto UpdateInterfacesFromSettings() -> void;

private:
  int m_bitWidth;
  int m_startBitWidth;
  Channel m_inputChannel;

private:
  AnalyzerSettingInterfaceInteger m_bitWidthInterface;
  AnalyzerSettingInterfaceChannel m_inputChannelInterface;
  AnalyzerSettingInterfaceInteger m_startBitWidthInterface;
};
