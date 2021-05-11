#if 0

#include "audioCtrl.h"
#include <stdlib.h>
#include <mutex>
#include "common/app_log.h"
#include "common/utils/utils.h"

using namespace EyeseeLinux;
using namespace std;

#define AUDIO_FILE_PATH "/usr/share/minigui/res/audio/"
#define AUDIO_FILE_ADAS_PATH "/usr/share/adas/adasAudio/"

AudioCtrl::AudioCtrl()
    : wav_parser_(NULL), wav_player_(NULL), inited_(false), volume_(0) {
  wav_file_list_.emplace(KEY1_SOUND, CON_2STRING(AUDIO_FILE_PATH, "key.wav"));
  wav_file_list_.emplace(AUTOPHOTO_SOUND,
                         CON_2STRING(AUDIO_FILE_PATH, "shutter.wav"));
  wav_file_list_.emplace(SHUTDOWN_SOUND,
                         CON_2STRING(AUDIO_FILE_PATH, "shutdown.wav"));
  wav_file_list_.emplace(TIMECOUNT_SOUND,
                         CON_2STRING(AUDIO_FILE_PATH, "autophoto.wav"));
  wav_file_list_.emplace(STARTUP_SOUND,
                         CON_2STRING(AUDIO_FILE_PATH, "startup.wav"));
#if 0
   wav_file_list_.emplace(ADAS_FCW_DANGER_SOUND, CON_2STRING(AUDIO_FILE_ADAS_PATH, "adas_front_2.wav"));
   wav_file_list_.emplace(ADAS_FCW_WARNING_SOUND, CON_2STRING(AUDIO_FILE_ADAS_PATH, "adas_front.wav"));
   wav_file_list_.emplace(ADAS_LDW_SOUND, CON_2STRING(AUDIO_FILE_ADAS_PATH, "adas_crimping.wav"));
#endif
  wav_parser_ = new WavParser();
  db_error("AudioCtrl::AudioCtrl");
  FILE *file = NULL;
  for (auto it : wav_file_list_) {
    file = fopen(it.second.c_str(), "rb");
    if (file != NULL) {
      PcmWaveData pcm_data;
      wav_parser_->GetPcmParam(file, pcm_data.param);
      pcm_data.size = wav_parser_->GetPcmDataSize(file);
      int size = pcm_data.size;
      if (size <= 0) {
        db_warn("no pcm data found in file: %s, size: %d", it.second.c_str(),
                size);
        fclose(file);
        continue;
      }
      if (size < 8192) {
        size = 8192;
      }
      pcm_data.buffer = malloc(size);
      if (pcm_data.buffer == NULL) {
        db_warn("malloc pcm data buffer failed, file: %s, alloc size: %d",
                it.second.c_str(), size);
        fclose(file);
        continue;
      }
      memset(pcm_data.buffer, 0, size);
      wav_parser_->GetPcmData(file, pcm_data.buffer, pcm_data.size);
      pcm_data.size = size;
      pcm_data_map_.emplace(it.first, pcm_data);
      fclose(file);
    } else {
      db_error("open wav file failed, file: %s, error: %s", it.second.c_str(),
               strerror(errno));
    }
  }
}

AudioCtrl::~AudioCtrl() {
  DeInitPlayer();
  db_error("AudioCtrl::AudioCtrl");
  if (wav_parser_ != NULL) delete wav_parser_;

  for (auto it : pcm_data_map_) {
    if (it.second.buffer) {
      free(it.second.buffer);
    }
  }

  pcm_data_map_.clear();
  wav_file_list_.clear();
}

int AudioCtrl::InitPlayer(int ao_card) {
  lock_guard<mutex> lock(init_mutex_);

  if (wav_player_ == NULL) {
    wav_player_ = new WavPlayer();
  }

  if (wav_player_->AOChannelInit(ao_card) < 0) {
    db_error("wav player ao channel init failed");
    inited_ = false;
    return -1;
  }

  inited_ = true;

  return 0;
}

int AudioCtrl::DeInitPlayer() {
  lock_guard<mutex> lock(init_mutex_);

  if (wav_player_ != NULL) {
    delete wav_player_;
    wav_player_ = NULL;
  }

  inited_ = false;

  return 0;
}

int AudioCtrl::SwitchAOCard(int ao_card) {
  DeInitPlayer();
  usleep(100 * 1000);
  InitPlayer(ao_card);

  return 0;
}

void AudioCtrl::SetBeepToneVolume(int val) {
  db_msg("the val is %d", val);
  volume_ = val;
}

int AudioCtrl::PlaySound(sounds_type type) {
  lock_guard<mutex> lock(init_mutex_);

  if (!inited_) {
    db_error("playsound failed, audio ctrl not init, try init directly");
    init_mutex_.unlock();
    if (InitPlayer(0) < 0) {
      return -1;
    }
    sleep(1);
  }
  wav_player_->AdjustBeepToneVolume(volume_);
  auto it = pcm_data_map_.find(type);
  if (it != pcm_data_map_.end()) {
    wav_player_->Play(pcm_data_map_[type]);
  } else {
    db_error("can not play key tone");
  }

  return 0;
}

#endif
