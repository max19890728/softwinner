/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <audio_hw.h>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "device_model/media/player/wav_player.h"

namespace Device {

class Audio {
 public:
  enum class Sound { start_up };

  static Audio& instance() {
    static Audio instance_;
    return instance_;
  }

 private:
  static const char* _audio_path_;
  std::mutex _mutex_;
  int _volume_;
  bool _is_loaded_;
  std::unique_ptr<EyeseeLinux::WavPlayer> _wav_player_ = nullptr;
  std::map<Device::Audio::Sound, EyeseeLinux::PcmWaveData> _pcm_data_map_;

 public:
  int volume();

  void volume(int new_volume);

  void InitPlayer(int ao_card);

  void DeinitPlayer();

  void SwitchAOCard(int new_ao_card);

  void Play(Device::Audio::Sound sound);

 private:
  Audio();
  ~Audio();
  Audio(const Audio&) = delete;
  Audio& operator=(const Audio&) = delete;

  void LoadAllSound();
};
}  // namespace Device
