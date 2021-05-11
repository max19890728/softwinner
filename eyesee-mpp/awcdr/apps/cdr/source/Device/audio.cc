/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/audio.h"

#include <fstream>
#include <ios>

#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "Device::Audio"

const char* Device::Audio::_audio_path_ = "/usr/share/minigui/res/audio";

Device::Audio::Audio() : _is_loaded_(false), _volume_(0) {
  db_info("");
  if (audioHw_Construct()) db_error("audioHw_Construct failed");
  LoadAllSound();
  InitPlayer(0);
  volume(100);
}

Device::Audio::~Audio() {}

void Device::Audio::LoadAllSound() {
  auto wav_parser = std::make_unique<EyeseeLinux::WavParser>();
  std::map<Device::Audio::Sound, std::string> all_sounds;
  all_sounds.emplace(Device::Audio::Sound::start_up, "startup.wav");
  for (const auto sound : all_sounds) {
    std::string file_path = std::string(_audio_path_) + "/" + sound.second;
    EyeseeLinux::PcmWaveData pcm_data;
#if 0
    std::ifstream file{file_path, std::ios::binary};
#else
    FILE* file = fopen(file_path.c_str(), "rb");
    if (file == NULL) {
      db_error("open file (%s) failed", file_path.c_str());
      continue;
    }
    wav_parser->GetPcmParam(file, pcm_data.param);
    int size = wav_parser->GetPcmDataSize(file);
    pcm_data.size = size;
    if (size <= 0) {
      db_error("no pcm data found");
      fclose(file);
      continue;
    }
    if (size < 8192) size = 8192;
    pcm_data.buffer = malloc(size);
    if (pcm_data.buffer == NULL) {
      db_warn("malloc pcm data buffer failed");
      fclose(file);
      continue;
    }
    memset(pcm_data.buffer, 0, size);
    wav_parser->GetPcmData(file, pcm_data.buffer, pcm_data.size);
    pcm_data.size = size;
    _pcm_data_map_.emplace(sound.first, pcm_data);
    fclose(file);
#endif
  }
}

int Device::Audio::volume() { return _volume_; }

void Device::Audio::volume(int new_volume) {
  _volume_ = new_volume;
  if (_wav_player_ != nullptr) {
    _wav_player_->AdjustBeepToneVolume(new_volume);
  }
}

void Device::Audio::InitPlayer(int ao_card) {
  std::lock_guard<std::mutex> lock(_mutex_);
  if (_wav_player_ == nullptr) {
    _wav_player_ = std::make_unique<EyeseeLinux::WavPlayer>();
  }
  if (_wav_player_->AOChannelInit(ao_card) < 0) {
    db_error("wav player ao channel init failed");
    _is_loaded_ = false;
  } else {
    _is_loaded_ = true;
  }
}

void Device::Audio::DeinitPlayer() {
  std::lock_guard<std::mutex> lock(_mutex_);
  if (_wav_player_ != nullptr) _wav_player_.reset();
  _is_loaded_ = false;
}

void Device::Audio::SwitchAOCard(int new_ao_card) {
  DeinitPlayer();
  usleep(100 * 1000);
  InitPlayer(new_ao_card);
}

void Device::Audio::Play(Device::Audio::Sound sound) {
  std::lock_guard<std::mutex> lock(_mutex_);
  auto data_iterator = _pcm_data_map_.find(sound);
  if (data_iterator != _pcm_data_map_.end()) {
    _wav_player_->Play(_pcm_data_map_[sound]);
  } else {
    db_error("not find sound.");
  }
}
