/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __DEMUX_MP4_H__
#define __DEMUX_MP4_H__

#include "Device/US363/Media/Mp4/mp4.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEMUX_TRAK_MAX    4

typedef struct demux_info_struct_h {
	// video
	uint32_t width;
	uint32_t height;
	uint16_t sps_len;
	uint8_t sps_buf[32];
	uint16_t pps_len;
	uint8_t pps_buf[32];

	// audio
	uint32_t sample_rate;
	uint32_t channel;
	uint32_t bits;
}demux_info_struct;
extern demux_info_struct demux_info;

typedef struct demux_mdhd_info_struct_h {
	char* ptr;
	mdhd_box_struct mdhd;
}demux_mdhd_info_struct;

typedef struct demux_hdlr_info_struct_h {
	char* ptr;
	hdlr_box_struct hdlr;
}demux_hdlr_info_struct;

typedef struct demux_stsd_info_struct_h {
	char* ptr;
	stsd_box_struct stsd;
}demux_stsd_info_struct;

typedef struct demux_video_entry_info_struct_h {
	char* ptr;
	visual_sample_entry_struct video_entry;
}demux_video_entry_info_struct;

typedef struct demux_audio_entry_info_struct_h {
	char* ptr;
	audio_sample_entry_struct audio_entry;
}demux_audio_entry_info_struct;

typedef struct demux_stts_entry_info_struct_h {
	char* ptr;
	char* sample_prt;
	stts_box_struct stts;
}demux_stts_entry_info_struct;

typedef struct demux_stss_entry_info_struct_h {
	char* ptr;
	char* sample_prt;
	stss_box_struct stss;
}demux_stss_entry_info_struct;

typedef struct demux_stsc_entry_info_struct_h {
	char* ptr;
	char* sample_prt;
	stsc_box_struct stsc;
}demux_stsc_entry_info_struct;

typedef struct demux_stsz_entry_info_struct_h {
	char* ptr;
	char* sample_prt;
	stsz_box_struct stsz;
}demux_stsz_entry_info_struct;

typedef struct demux_stco_entry_info_struct_h {
	char* ptr;
	char* sample_prt;
	stco_box_struct stco;
}demux_stco_entry_info_struct;

typedef struct demux_trak_info_struct_h {
	int flag;
	int size;
	char* ptr;

	demux_mdhd_info_struct mdhd_info;
	demux_hdlr_info_struct hdlr_info;
	demux_stsd_info_struct stsd_info;
	demux_video_entry_info_struct video_entry_info;
	demux_audio_entry_info_struct audio_entry_info;
	demux_stts_entry_info_struct stts_info;
	demux_stss_entry_info_struct stss_info;
	demux_stsc_entry_info_struct stsc_info;
	demux_stsz_entry_info_struct stsz_info;
	demux_stco_entry_info_struct stco_info;

}demux_trak_info_struct;
extern demux_trak_info_struct trak_box[DEMUX_TRAK_MAX];


int demux_mp4(char* src_path);
void demux_destroy(void);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__DEMUX_MP4_H__