
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>

#include "Device/US363/Media/Tinyalsa/asoundlib.h"
//tmp #include "us360.h"
#include "Device/US363/Media/Mp4/mp4.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::Tinycap"

#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164

#define FORMAT_PCM 1


FILE *file;
struct wav_header header;
//unsigned int card;
//unsigned int device;
//unsigned int channels;
//unsigned int rate;
//unsigned int bits;
//unsigned int frames;
//unsigned int period_size;
//unsigned int period_count;

//struct pcm_config config;
struct pcm *pcm = NULL;
unsigned char *tinycap_buf;
unsigned int tinycap_size;
//unsigned int bytes_read = 0;

struct wav_header {
    uint32_t riff_id;
    uint32_t riff_sz;
    uint32_t riff_fmt;
    uint32_t fmt_id;
    uint32_t fmt_sz;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    uint32_t data_id;
    uint32_t data_sz;
};

int capturing = 1;

int capture_sample(FILE *file, unsigned int card, unsigned int device,
                            unsigned int channels, unsigned int rate,
                            unsigned int bits, unsigned int period_size,
                            unsigned int period_count, int en, int fps);

void sigint_handler(int sig)
{
    capturing = sig;
}

int recorderWAV(int en, int fps)
{
    //FILE *file;
    //struct wav_header header;
    unsigned int card = 2;
    unsigned int device = 0;
    unsigned int channels = 1;
    unsigned int rate = 44100;
    unsigned int bits = 16;
    unsigned int period_size = 1024;
    unsigned int period_count = 4;
    int frames = 0;

    // 44100 * 1 * 16 / 8 / 30 = 2940
    // 48000 * 1 * 16 / 8 / 30 = 3200
	period_size  = rate * channels * bits / 8 / fps / period_count;

    if(en == 0)
    {
    	//file = fopen("/mnt/sdcard/test.wav", "wb");
    	//if (!file) {
    	//    db_error("Unable to create file (%d)\n", en);
    	//    return 1;
    	//}

    	header.riff_id = ID_RIFF;
    	header.riff_sz = 0;
    	header.riff_fmt = ID_WAVE;
    	header.fmt_id = ID_FMT;
    	header.fmt_sz = 16;
    	header.audio_format = FORMAT_PCM;
    	header.num_channels = channels;
    	header.sample_rate = rate;
    	header.bits_per_sample = bits;
    	header.byte_rate = (header.bits_per_sample / 8) * channels * rate;
    	header.block_align = channels * (header.bits_per_sample / 8);
    	header.data_id = ID_DATA;

    	// leave enough room for header
    	//fseek(file, sizeof(struct wav_header), SEEK_SET);
    }

    if(en == 0 || en == 1)
    {
    	//if (!file) {
    	//    db_error("Unable to create file (%d)\n", en);
    	//    return 1;
    	//}
    	// install signal handler and begin capturing
    	//signal(SIGINT, sigint_handler);
    	//db_debug("capture_sample star %ld\n", getCurrentTime());
    	frames = capture_sample(file, card, device, header.num_channels,
                            	header.sample_rate, header.bits_per_sample,
                            	period_size, period_count, en, fps);
    	//db_debug("Captured %d frames %ld\n", frames, getCurrentTime());
    }

    if(en == -1 || frames <= 0)
    {
    	//if (!file) {
    	//    db_error("Unable to create file (%d)\n", en);
    	//    return 1;
    	//}
    	// write header now all information is known
    	//header.data_sz = frames * header.block_align;
    	//fseek(file, 0, SEEK_SET);
    	//fwrite(&header, sizeof(struct wav_header), 1, file);

    	if(tinycap_buf) {
    		free(tinycap_buf);
    		tinycap_buf = NULL;
    	}

    	if(pcm) {
    		pcm_close(pcm);
    		pcm = NULL;
    	}
    	return -1;
    }

    return 0;
}
extern void set_rtsp_audio_rate(int rate);              // live555_init.c
extern int copy_to_rtsp_audio_buff(char *buf, int len);

extern void set_rtmp_audio_rate(int rate);				// us360.c
extern void set_rtmp_audio_channel(int channel);
extern void set_rtmp_audio_bit(int bit);
extern int copy_to_rtmp_audio_buff(char *buf, int len);

int capture_sample(FILE *file, unsigned int card, unsigned int device,
                            unsigned int channels, unsigned int rate,
                            unsigned int bits, unsigned int period_size,
                            unsigned int period_count, int en, int fps)
{
    struct pcm_config config;
    //struct pcm *pcm;
    //char *tinycap_buf;
    //unsigned int size;
    unsigned int bytes_read = 0;
    int ret, i, asize;

    if(en == 0 && pcm == NULL)
    {
    	config.channels = channels;
    	config.rate = rate;
    	config.period_size = period_size;
    	config.period_count = period_count;
    	if (bits == 32)
    		config.format = PCM_FORMAT_S32_LE;
    	else if (bits == 16)
    		config.format = PCM_FORMAT_S16_LE;
    	config.start_threshold = 0;
    	config.stop_threshold = 0;
    	config.silence_threshold = 0;


		pcm = pcm_open(card, device, PCM_IN, &config, fps);
    	if (!pcm || !pcm_is_ready(pcm)) {
    		db_error("Unable to open PCM device (%s)\n", pcm_get_error(pcm));
    		audio_rate = -1;
    		return 0;
    	}
        //ret = pcm_get_config(pcm, &config);
        db_debug("capture_sample() channels=%d, rate=%d, size=%d, count=%d\n", config.channels, config.rate, config.period_size, config.period_count);
        audio_rate = config.rate;
//tmp        set_rtsp_audio_rate(audio_rate);          // rex+ 170703
//tmp        set_rtmp_audio_rate(audio_rate);		  // kiuno+ 170717
//tmp        set_rtmp_audio_channel(config.channels);
//tmp        set_rtmp_audio_bit(bits);
    	tinycap_size = pcm_get_buffer_size(pcm);
    	db_debug("capture_sample() tinycap_size %d\n", tinycap_size);
    }

	if(tinycap_buf == NULL)
	{
		db_debug("capture_sample() tinycap_buf malloc %d\n", tinycap_size);
		tinycap_buf = malloc(tinycap_size);
		if (!tinycap_buf) {
			db_error("Unable to allocate %d bytes\n", tinycap_size);
			pcm_close(pcm);
			return 0;
		}
	}

	//while (capturing && !pcm_read(pcm, tinycap_buf, size))
    ret = pcm_read(pcm, tinycap_buf, tinycap_size);
    if(ret == 0)
    {
        //if (fwrite(tinycap_buf, 1, tinycap_size, file) != tinycap_size) {
           // db_error("Error capturing sample\n");
            //break;
        //}
    	//db_debug("capture_sample() bytes_read += %d\n", tinycap_size);
        bytes_read += tinycap_size;
        for(i = 0; i < 10; i++){
//tmp            asize = copy_to_rtsp_audio_buff(tinycap_buf, tinycap_size);
//tmp            asize = copy_to_rtmp_audio_buff(tinycap_buf, tinycap_size);
            if(asize >= 0) break;
            else usleep(3000);
        }
        if(i >= 10)
            db_error("capture_sample: err! i=%d asize=%d\n", i, asize);
    }
	else {
        /*if(tinycap_size != 0){
            for(i=0;i<tinycap_size;i++)
                tinycap_buf[i] = 0;
            bytes_read += tinycap_size;
        }*/
        //db_error("capture_sample() pcm_read err!\n");
        return -1;
	}

    //free(tinycap_buf);
    //tinycap_buf = NULL;
    //pcm_close(pcm);
    //pcm = NULL;
    return bytes_read / ( (bits / 8) * channels);
}

