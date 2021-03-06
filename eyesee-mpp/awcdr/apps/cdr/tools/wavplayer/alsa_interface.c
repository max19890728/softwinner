/******************************************************************************
  Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 ******************************************************************************
  File Name     : alsa_interface.c
  Version       : Initial Draft
  Author        : Allwinner BU3-PD2 Team
  Created       : 2016/04/19
  Last Modified :
  Description   : mpi functions implement
  Function List :
  History       :
******************************************************************************/

#define LOG_NDEBUG 0
#define LOG_TAG "alsa_interface"
#include <utils/plat_log.h>

#include "alsa_interface.h"

//static long gVolume;
static char aioDebugfsPath[64] = {0};
static unsigned int updateCnt = 0;

enum {
    aiDEBit = 0,
    aiCCBit = 1,
    aiCTBit = 2,
    aiSRBit = 3,
    aiBWBit = 4,
    aiTCBit = 5,
    aiBMBit = 6,
    aiVOLBit = 7,

    aoDEBit = 8,
    aoCCBit = 9,
    aoCTBit = 10,
    aoSRBit = 11,
    aoBWBit = 12,
    aoTCBit = 13,
    aoBMBit = 14,
    aoVOLBit = 15,
};

// 0-none item need update; !0-some items need update.
static unsigned int aioDebugUpdataFlag = 0;
enum {
    aiDevEnableFlag = 1<<aiDEBit,     //ai dev updata falg
    aiChnCntFlag = 1<<aiCCBit,
    aiCardTypeFlag = 1<<aiCTBit,
    aiSampleRateFlag = 1<<aiSRBit,
    aiBitWidthFlag = 1<<aiBWBit,
    aiTrackCntFlag = 1<<aiTCBit,
    aibMuteFlag = 1<<aiBMBit,
    aiVolumeFlag = 1<<aiVOLBit,
    aoDevEnableFlag = 1<<aoDEBit,     //ao dev updata falg
    aoChnCntFlag = 1<<aoCCBit,
    aoCardTypeFlag = 1<<aoCTBit,
    aoSampleRateFlag = 1<<aoSRBit,
    aoBitWidthFlag = 1<<aoBWBit,
    aoTrackCntFlag = 1<<aoTCBit,
    aobMuteFlag = 1<<aoBMBit,
    aoVolumeFlag = 1<<aoVOLBit,
};

static int aiDevEnable;
static int aiChnCnt;
static int aiCardType;     //0-codec; 1-linein
static int aiSampleRate;
static int aiBitWidth;
static int aiTrackCnt;
static int aibMute;
static int aiVolume;

static int aoDevEnable;
static int aoChnCnt;
static int aoCardType;     //0-codec; 1-hdmi
static int aoSampleRate;
static int aoBitWidth;
static int aoTrackCnt;
static int aobMute;
static int aoVolume;

int UpdatebugfsInfo()
{
    //system("cat /proc/mounts | grep debugfs | awk '{print $2}'");
    // /proc/sys/debug/mpp/sunxi-aio
    if (aioDebugUpdataFlag)
    {
        if (updateCnt++ % 100 == 0)
        {
            FILE *stream;
            char *cmd = "cat /proc/mounts | grep debugfs | awk '{print $2}'";
            stream = popen(cmd, "r");
            char path[64] = {0};
            fread(path, 1, sizeof(path), stream);
            pclose(stream);
            path[strlen(path)-1] = 0;   //\n -> \0
            strcat(path, "/mpp/sunxi-aio");
            strncpy(aioDebugfsPath, path, sizeof(path));
        }
        if (access(aioDebugfsPath, F_OK) != 0)
        {
            //alogv("sunxi-aio debugfs path not find!");
            return -1;
        }

        char buf[64] = {0};

        // format: echo X YZ > Path, here X must match code with aio.c
        // ai dev info
        if (aioDebugUpdataFlag & aiDevEnableFlag)
        {
            aioDebugUpdataFlag &= ~aiDevEnableFlag;
            memset(buf, 0, sizeof(buf));
            sprintf(buf, "echo 0 %d > %s\n", aiDevEnable, aioDebugfsPath);
            system(buf);
        }
        if (aioDebugUpdataFlag & aiChnCntFlag)
        {
            aioDebugUpdataFlag &= ~aiChnCntFlag;
            memset(buf, 0, sizeof(buf));
            sprintf(buf, "echo 1 %d > %s\n", aiChnCnt, aioDebugfsPath);
            system(buf);
        }
        if (aioDebugUpdataFlag & aiCardTypeFlag)
        {
            aioDebugUpdataFlag &= ~aiCardTypeFlag;
            memset(buf, 0, sizeof(buf));
            sprintf(buf, "echo 2 %d > %s\n", aiCardType, aioDebugfsPath);
            system(buf);
        }
        if (aioDebugUpdataFlag & aiSampleRateFlag)
        {
            aioDebugUpdataFlag &= ~aiSampleRateFlag;
            memset(buf, 0, sizeof(buf));
            sprintf(buf, "echo 3 %d > %s\n", aiSampleRate, aioDebugfsPath);
            system(buf);
        }
        if (aioDebugUpdataFlag & aiBitWidthFlag)
        {
            aioDebugUpdataFlag &= ~aiBitWidthFlag;
            memset(buf, 0, sizeof(buf));
            sprintf(buf, "echo 4 %d > %s\n", aiBitWidth, aioDebugfsPath);
            system(buf);
        }
        if (aioDebugUpdataFlag & aiTrackCntFlag)
        {
            aioDebugUpdataFlag &= ~aiTrackCntFlag;
            memset(buf, 0, sizeof(buf));
            sprintf(buf, "echo 5 %d > %s\n", aiTrackCnt, aioDebugfsPath);
            system(buf);
        }
        if (aioDebugUpdataFlag & aibMuteFlag)
        {
            aioDebugUpdataFlag &= ~aibMuteFlag;
            memset(buf, 0, sizeof(buf));
            sprintf(buf, "echo 6 %d > %s\n", aibMute, aioDebugfsPath);
            system(buf);
        }
        if (aioDebugUpdataFlag & aiVolumeFlag)
        {
            aioDebugUpdataFlag &= ~aiVolumeFlag;
            memset(buf, 0, sizeof(buf));
            sprintf(buf, "echo 7 %d > %s\n", aiVolume, aioDebugfsPath);
            system(buf);
        }

        // ao dev info
        if (aioDebugUpdataFlag & aoDevEnableFlag)
        {
            aioDebugUpdataFlag &= ~aoDevEnableFlag;
            memset(buf, 0, sizeof(buf));
            sprintf(buf, "echo 8 %d > %s\n", aoDevEnable, aioDebugfsPath);
            system(buf);
        }
        if (aioDebugUpdataFlag & aoChnCntFlag)
        {
            aioDebugUpdataFlag &= ~aoChnCntFlag;
            memset(buf, 0, sizeof(buf));
            sprintf(buf, "echo 9 %d > %s\n", aoChnCnt, aioDebugfsPath);
            system(buf);
        }
        if (aioDebugUpdataFlag & aoCardTypeFlag)
        {
            aioDebugUpdataFlag &= ~aoCardTypeFlag;
            memset(buf, 0, sizeof(buf));
            sprintf(buf, "echo a %d > %s\n", aoCardType, aioDebugfsPath);
            system(buf);
        }
        if (aioDebugUpdataFlag & aoSampleRateFlag)
        {
            aioDebugUpdataFlag &= ~aoSampleRateFlag;
            memset(buf, 0, sizeof(buf));
            sprintf(buf, "echo b %d > %s\n", aoSampleRate, aioDebugfsPath);
            system(buf);
        }
        if (aioDebugUpdataFlag & aoBitWidthFlag)
        {
            aioDebugUpdataFlag &= ~aoBitWidthFlag;
            memset(buf, 0, sizeof(buf));
            sprintf(buf, "echo c %d > %s\n", aoBitWidth, aioDebugfsPath);
            system(buf);
        }
        if (aioDebugUpdataFlag & aoTrackCntFlag)
        {
            aioDebugUpdataFlag &= ~aoTrackCntFlag;
            memset(buf, 0, sizeof(buf));
            sprintf(buf, "echo d %d > %s\n", aoTrackCnt, aioDebugfsPath);
            system(buf);
        }
        if (aioDebugUpdataFlag & aobMuteFlag)
        {
            aioDebugUpdataFlag &= ~aobMuteFlag;
            memset(buf, 0, sizeof(buf));
            sprintf(buf, "echo e %d > %s\n", aibMute, aioDebugfsPath);
            system(buf);
        }
        if (aioDebugUpdataFlag & aoVolumeFlag)
        {
            aioDebugUpdataFlag &= ~aoVolumeFlag;
            memset(buf, 0, sizeof(buf));
            sprintf(buf, "echo f %d > %s\n", aoVolume, aioDebugfsPath);
            system(buf);
        }
        aioDebugUpdataFlag = 0;
    }

    return 0;
}

int clearDebugfsInfoByHwDev(int pcmFlag)
{
    if (pcmFlag == 0)
    {
        aiDevEnable = 0;
        aiChnCnt = 0;
        aiCardType = 0;     //0-codec; 1-linein
        aiSampleRate = 0;
        aiBitWidth = 0;
        aiTrackCnt = 0;
        aibMute = 0;
        aiVolume = 0;
        aioDebugUpdataFlag |= 0xff;
    }
    else
    {
        aoDevEnable = 0;
        aoChnCnt = 0;
        aoCardType = 0;     //0-codec; 1-hdmi
        aoSampleRate = 0;
        aoBitWidth = 0;
        aoTrackCnt = 0;
        aobMute = 0;
        aoVolume = 0;
        aioDebugUpdataFlag |= 0xff00;
    }
    UpdatebugfsInfo();

    return 0;
}

// pcmFlag: 0-cap update; 1-play update
void updateDebugfsByChnCnt(int pcmFlag, int cnt)
{
    if (pcmFlag == 0)
    {
        aiChnCnt = cnt;
        aioDebugUpdataFlag |= aiChnCntFlag;
    }
    else
    {
        aoChnCnt = cnt;
        aioDebugUpdataFlag |= aoChnCntFlag;
    }
}

int alsaSetPcmParams(PCM_CONFIG_S *pcmCfg)
{
    snd_pcm_hw_params_t *params;
    snd_pcm_sw_params_t *swparams;
    int err;
    unsigned int rate, bufTime, periodTime;
//    snd_pcm_uframes_t startThreshold, stopThreshold;
    int dir = 0;

    if (pcmCfg->handle == NULL) {
        aloge("PCM is not open yet!");
        return -1;
    }
    alogd("set pcm params");

    snd_pcm_hw_params_alloca(&params);
//    snd_pcm_sw_params_alloca(&swparams);
    err = snd_pcm_hw_params_any(pcmCfg->handle, params);
    if (err < 0) {
        aloge("Broken configuration for this PCM: no configurations available");
        return -1;
    }

    err = snd_pcm_hw_params_set_access(pcmCfg->handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        aloge("Access type not available");
        return -1;
    }

    err = snd_pcm_hw_params_set_format(pcmCfg->handle, params, pcmCfg->format);
    if (err < 0) {
        aloge("Sample format not available");
        return -1;
    }

    err = snd_pcm_hw_params_set_channels(pcmCfg->handle, params, pcmCfg->chnCnt);
    if (err < 0) {
        aloge("Channels count not available");
        return -1;
    }

    rate = pcmCfg->sampleRate;
    err = snd_pcm_hw_params_set_rate_near(pcmCfg->handle, params, &pcmCfg->sampleRate, NULL);
    if (err < 0) {
        aloge("set_rate_near error!");
        return -1;
    }
    if (rate != pcmCfg->sampleRate) {
        alogd("required sample_rate %d is not supported, use %d instead", rate, pcmCfg->sampleRate);
    }

    snd_pcm_uframes_t periodSize = 1024;
    err = snd_pcm_hw_params_set_period_size_near(pcmCfg->handle, params, &periodSize, &dir);
    if (err < 0) {
        aloge("set_period_size_near error!");
        return -1;
    }
    // double 1024-sample capacity -> 4
    snd_pcm_uframes_t bufferSize = periodSize * pcmCfg->chnCnt * snd_pcm_format_physical_width(pcmCfg->format) / 2;
    err = snd_pcm_hw_params_set_buffer_size_near(pcmCfg->handle, params, &bufferSize);
    if (err < 0) {
        aloge("set_buffer_size_near error!");
        return -1;
    }

    err = snd_pcm_hw_params(pcmCfg->handle, params);
    if (err < 0) {
        aloge("Unable to install hw params");
        return -1;
    }

    snd_pcm_hw_params_get_period_size(params, &pcmCfg->chunkSize, 0);
    snd_pcm_hw_params_get_buffer_size(params, &pcmCfg->bufferSize);
    if (pcmCfg->chunkSize == pcmCfg->bufferSize) {
        aloge("Can't use period equal to buffer size (%lu == %lu)", pcmCfg->chunkSize, pcmCfg->bufferSize);
        return -1;
    }

    pcmCfg->bitsPerSample = snd_pcm_format_physical_width(pcmCfg->format);
    pcmCfg->significantBitsPerSample = snd_pcm_format_width(pcmCfg->format);
    pcmCfg->bitsPerFrame = pcmCfg->bitsPerSample * pcmCfg->chnCnt;
    pcmCfg->chunkBytes = pcmCfg->chunkSize * pcmCfg->bitsPerFrame / 8;

    alogd("----------------ALSA setting----------------");
    alogd(">>Channels:   %4d, BitWidth:  %4d, SampRate:   %4d", pcmCfg->chnCnt, pcmCfg->significantBitsPerSample, pcmCfg->sampleRate);
    alogd(">>ChunkBytes: %4d, ChunkSize: %4d, BufferSize: %4d", pcmCfg->chunkBytes, (int)pcmCfg->chunkSize, (int)pcmCfg->bufferSize);

    return 0;
}

int alsaOpenPcm(PCM_CONFIG_S *pcmCfg, const char *card, int pcmFlag)
{
    snd_pcm_info_t *info;
    snd_pcm_stream_t stream;
    int err;

    if (pcmCfg->handle != NULL) {
        alogw("PCM is opened already!");
        return 0;
    }
    alogd("open pcm! card:[%s], pcmFlag:[%d](0-cap;1-play)", card, pcmFlag);

    // 0-cap; 1-play
    stream = (pcmFlag == 0) ? SND_PCM_STREAM_CAPTURE : SND_PCM_STREAM_PLAYBACK;

    snd_pcm_info_alloca(&info);

    err = snd_pcm_open(&pcmCfg->handle, card, stream, 0);
    if (err < 0) {
        aloge("audio open error: %s", snd_strerror(err));
        return -1;
    }
    if ((err = snd_pcm_info(pcmCfg->handle, info)) < 0) {
        aloge("snd_pcm_info error: %s", snd_strerror(err));
        return -1;
    }

    if (stream == SND_PCM_STREAM_CAPTURE) {
        aiDevEnable = 1;
        aiCardType = 0;
        aiSampleRate = pcmCfg->sampleRate;
        aiBitWidth = pcmCfg->bitsPerSample;
        aiTrackCnt = pcmCfg->chnCnt;
        aioDebugUpdataFlag |= aiDevEnableFlag | aiCardTypeFlag | aiSampleRateFlag | aiBitWidthFlag | aiTrackCntFlag;
    } else if (stream == SND_PCM_STREAM_PLAYBACK) {
        aoDevEnable = 1;
        aoCardType = strncmp(card, "default", 7)==0?0:1;
        aoSampleRate = pcmCfg->sampleRate;
        aoBitWidth = pcmCfg->bitsPerSample;
        aoTrackCnt= pcmCfg->chnCnt;
        aioDebugUpdataFlag |= aoDevEnableFlag | aoCardTypeFlag | aoSampleRateFlag | aoBitWidthFlag | aoTrackCntFlag;
    }

    return 0;
}

void alsaClosePcm(PCM_CONFIG_S *pcmCfg, int pcmFlag)
{
    //alogd("close pcm");
    clearDebugfsInfoByHwDev(pcmFlag);

    if (pcmCfg->handle == NULL) {
        aloge("PCM is not open yet!");
        return;
    }
    snd_pcm_close(pcmCfg->handle);
    pcmCfg->handle = NULL;
}

ssize_t alsaReadPcm(PCM_CONFIG_S *pcmCfg, void *data, size_t rcount)
{
    ssize_t ret;
    ssize_t result = 0;

    if (rcount != pcmCfg->chunkSize)
        rcount = pcmCfg->chunkSize;

    if (pcmCfg == NULL || data == NULL) {
        aloge("invalid input parameter(pcmCfg=%p, data=%p)!", pcmCfg, data);
        return -1;
    }

    while (rcount > 0) {
        UpdatebugfsInfo();
        ret = snd_pcm_readi(pcmCfg->handle, data, rcount);
        if (ret == -EAGAIN || (ret >= 0 && (size_t)ret < rcount)) {
            snd_pcm_wait(pcmCfg->handle, 100);
        } else if (ret == -EPIPE) {
            alogd("xrun(%s)!", strerror(errno));
            snd_pcm_prepare(pcmCfg->handle);
        } else if (ret == -ESTRPIPE) {
            alogd("need suspend!");
        } else if (ret < 0) {
            aloge("read error: %s", snd_strerror(ret));
            return -1;
        }

        if (ret > 0) {
            result += ret;
            rcount -= ret;
            data += ret * pcmCfg->bitsPerFrame / 8;
        }
    }

    return result;
}

ssize_t alsaWritePcm(PCM_CONFIG_S *pcmCfg, void *data, size_t wcount)
{
    ssize_t ret;
    ssize_t result = 0;

    while (wcount > 0) {
        UpdatebugfsInfo();
		if(pcmCfg->handle == NULL || data==NULL)
		{
			aloge("handle or data is NULL handle:%p, data:%p", pcmCfg->handle, data);
			return result;
		}
        ret = snd_pcm_writei(pcmCfg->handle, data, wcount);
        if (ret == -EAGAIN || (ret >= 0 && (size_t)ret < wcount)) {
            snd_pcm_wait(pcmCfg->handle, 100);
        } else if (ret == -EPIPE) {
            //alogv("xrun!");
            snd_pcm_prepare(pcmCfg->handle);
        } else if (ret == -EBADFD) {
            //alogw("careful! current pcm state: %d", snd_pcm_state(pcmCfg->handle));
            snd_pcm_prepare(pcmCfg->handle);
        } else if (ret == -ESTRPIPE) {
            alogd("need suspend!");
        } else if (ret < 0) {
            aloge("write error! ret:%d, %s", ret, snd_strerror(ret));
            return -1;
        }

        if (ret > 0) {
            result += ret;
            wcount -= ret;
            data += ret * pcmCfg->bitsPerFrame / 8;
        }
    }

    return result;
}

int alsaDrainPcm(PCM_CONFIG_S *pcmCfg)
{
    int err = 0;

    err = snd_pcm_drain(pcmCfg->handle);
    if (err != 0){
        aloge("drain pcm err! err=%d", err);
    }

    return err;
}

int alsaOpenMixer(AIO_MIXER_S *mixer, const char *card, long vol_val)
{
    snd_mixer_selem_id_t *sid;
    snd_mixer_elem_t *elem;
    int err = 0;

    if (mixer->handle != NULL) {
        return 0;
    }
    alogd("open mixer");

    snd_mixer_selem_id_alloca(&sid);

    err = snd_mixer_open(&mixer->handle, 0);
    if (err < 0) {
        aloge("Mixer %s open error: %s\n", card, snd_strerror(err));
        return err;
    }

    err = snd_mixer_attach(mixer->handle, card);
    if (err < 0) {
        aloge("Mixer %s attach error: %s\n", card, snd_strerror(err));
        goto ERROR;
    }

    err = snd_mixer_selem_register(mixer->handle, NULL, NULL);
    if (err < 0) {
        aloge("Mixer %s register error: %s\n", card, snd_strerror(err));
        goto ERROR;
    }

    err = snd_mixer_load(mixer->handle);
    if (err < 0) {
        aloge("Mixer %s load error: %s\n", card, snd_strerror(err));
        goto ERROR;
    }

    for (elem = snd_mixer_first_elem(mixer->handle); elem; elem = snd_mixer_elem_next(elem)) {
        snd_mixer_selem_get_id(elem, sid);
        const char *elem_name = snd_mixer_selem_get_name(elem);
        if (!strcmp(elem_name, AUDIO_LINEOUT_SWITCH) ||
            !strcmp(elem_name, AUDIO_DACL_MIXER) ||
            !strcmp(elem_name, AUDIO_DACR_MIXER) ||
            !strcmp(elem_name, AUDIO_LOUTPUT_MIXER) ||
            !strcmp(elem_name, AUDIO_ROUTPUT_MIXER) ||
            !strcmp(elem_name, AUDIO_AD0L_MIXER) ||
            !strcmp(elem_name, AUDIO_AD0R_MIXER) ||
            !strcmp(elem_name, AUDIO_LADC_MIC1_SWITCH) ||
            !strcmp(elem_name, AUDIO_RADC_MIC1_SWITCH) ||
            !strcmp(elem_name, AUDIO_LINEINL_SWITCH) ||
            !strcmp(elem_name, AUDIO_LINEINR_SWITCH) ) {
            // open switch
            snd_mixer_selem_set_playback_switch(elem, 0, 1);
        } else if (!strcmp(elem_name, AUDIO_LINEOUT_VOL)) {
            snd_mixer_selem_set_playback_volume(elem, 0, 31);
        } else if (!strcmp(elem_name, "AIF1 DAC timeslot 0 volume")) {
            aloge("--->AIF1 DAC timeslot 0 volume:%d ", vol_val);
            snd_mixer_selem_set_playback_volume(elem, 0, vol_val);
        }
    }

    return err;

ERROR:
    snd_mixer_close(mixer->handle);
    mixer->handle = NULL;
    return err;
}

void alsaCloseMixer(AIO_MIXER_S *mixer)
{
    if (mixer->handle == NULL) {
        return;
    }
    alogd("close mixer");

    snd_mixer_close(mixer->handle);
    mixer->handle = NULL;
}

int alsaMixerSetVolume(AIO_MIXER_S *mixer, int playFlag, long value)
{
    int err = 0;

    if (mixer->handle == NULL) {
        return -1;
    }

    snd_mixer_elem_t *elem;
    for (elem = snd_mixer_first_elem(mixer->handle); elem; elem = snd_mixer_elem_next(elem)) {
        const char *elem_name = snd_mixer_selem_get_name(elem);
        alogd("--> elem_name:%s", elem_name);
        if (playFlag && !strcmp(elem_name, "AIF1 DAC timeslot 0 volume")) {
            err = snd_mixer_selem_set_playback_volume(elem, 0, value);
            alogd("-------->AIF1 DAC timelot:%ld, err:%d", value, err);
            aoVolume = value;
            aioDebugUpdataFlag |= aoVolumeFlag;
            break;
        }
    }
    return err;
}

int alsaMixerGetVolume(AIO_MIXER_S *mixer, int playFlag, long *value)
{
    int err = 0;

    if (mixer->handle == NULL) {
        return -1;
    }

    snd_mixer_elem_t *elem;
    for (elem = snd_mixer_first_elem(mixer->handle); elem; elem = snd_mixer_elem_next(elem)) {
        const char *elem_name = snd_mixer_selem_get_name(elem);
        if (playFlag && !strcmp(elem_name, AUDIO_LINEOUT_VOL)) {
            long realVal;
            err = snd_mixer_selem_get_playback_volume(elem, 0, &realVal);
            // scale from AUDIO_VOLUME_MAX to 100
            *value = realVal * 100 / AUDIO_VOLUME_MAX;
            alogd("playback getVolume:%ld, dst:%ld, err:%d", realVal, *value, err);
            aoVolume = *value;
            aioDebugUpdataFlag |= aoVolumeFlag;
            break;
        } else if (!playFlag && !strcmp(elem_name, AUDIO_ADC_VOLUME)) {
            long realVal;
            err = snd_mixer_selem_get_capture_volume(elem, 0, &realVal);
            // scale from 110-160 to 0-100
            *value = (realVal-110) * 2;
            alogd("capture getVolume:%ld, dst:%ld, err:%d", realVal, *value, err);
            aiVolume = *value;
            aioDebugUpdataFlag |= aiVolumeFlag;
            break;
        }
    }
    return err;
}

int alsaMixerSetMute(AIO_MIXER_S *mixer, int playFlag, int bEnable)
{
    int err = 0;

    if (mixer->handle == NULL) {
        return -1;
    }

    snd_mixer_elem_t *elem;
    for (elem = snd_mixer_first_elem(mixer->handle); elem; elem = snd_mixer_elem_next(elem)) {
        const char *elem_name = snd_mixer_selem_get_name(elem);
        if (playFlag && !strcmp(elem_name, AUDIO_LINEOUT_SWITCH)) {
            alogd("set player master-volume switch state: %d", bEnable);
            if (bEnable) {
                err = snd_mixer_selem_set_playback_switch(elem, 0, 0);
            } else {
                err = snd_mixer_selem_set_playback_switch(elem, 0, 1);
            }
            aobMute = bEnable;
            aioDebugUpdataFlag |= aobMuteFlag;
            break;
        } else if (!playFlag && !strcmp(elem_name, AUDIO_ADC_VOLUME)) {
            alogd("set capture ADC volme state: %d", bEnable);
            if (bEnable) {
                err = snd_mixer_selem_set_capture_volume(elem, 0, 0);
            } else {
                err = snd_mixer_selem_set_capture_volume(elem, 0, 160);
            }
            aibMute = bEnable;
            aioDebugUpdataFlag |= aibMuteFlag;
            break;
        }
    }
    return err;
}

int alsaMixerGetMute(AIO_MIXER_S *mixer, int playFlag, int *pVolVal)
{
    int err = 0;

    if (mixer->handle == NULL) {
        return -1;
    }

    snd_mixer_elem_t *elem;
    for (elem = snd_mixer_first_elem(mixer->handle); elem; elem = snd_mixer_elem_next(elem)) {
        const char *elem_name = snd_mixer_selem_get_name(elem);
        if (playFlag && !strcmp(elem_name, AUDIO_LINEOUT_SWITCH)) {
            err = snd_mixer_selem_get_playback_switch(elem, 0, pVolVal);
            alogd("get master-volume (0-mute; 1-unmute) switch state: %d", *pVolVal);
            aobMute = (*pVolVal==0?1:0);
            aioDebugUpdataFlag |= aobMuteFlag;
            break;
        } else if (!playFlag && !strcmp(elem_name, AUDIO_ADC_VOLUME)) {
            long tmpVol;
            err = snd_mixer_selem_get_capture_volume(elem, 0, &tmpVol);
            *pVolVal = tmpVol<130?0:1;
            alogd("get capture ADC volume(%ld) (0-mute; 1-unmute) switch state: %d", tmpVol, *pVolVal);
            aibMute = (tmpVol<130?1:0);
            aioDebugUpdataFlag |= aibMuteFlag;
            break;
        }
    }
    return err;
}
