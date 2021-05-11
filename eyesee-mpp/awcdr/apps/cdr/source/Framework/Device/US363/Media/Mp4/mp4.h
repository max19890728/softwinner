/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __MP4_H__
#define __MP4_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SWAP32(x) (((x) >> 24) | (((x)&0x00ff0000) >> 8) | (((x)&0x0000ff00) << 8) | ((x) << 24))
#define SWAP24(x) (((x) >> 16) | ((x)&0x00ff00) | ((x) << 16) )
#define SWAP16(x) (((x) >> 8) | ((x&0xff) << 8))
#define CC(s) (*((uint32_t *)(s)))

/* 1GB, 錄影單支檔案最大容量 */
#define REC_PER_FILE_SIZE_MP4 0x40000000

#define MP4_HEADER_BUF_SIZE 0x400
#define STTS_E_BUF_SIZE 0x6B800
#define STSS_E_BUF_SIZE 0x37000
#define STSC_E_BUF_SIZE 0xD4800
#define STSZ_E_BUF_SIZE 0x37000
#define STCO_E_BUF_SIZE 0x37000

#define ALETA_MODE_GLOBAL_STR "gbal"
#define ALETA_MODE_FRONT_STR  "font"
#define ALETA_MODE_360_STR    "a360"
#define ALETA_MODE_180_STR    "a180"
#define ALETA_MODE_4SPLIT_STR "4spl"

/* 30MB, SD Card 最小剩餘空間 */
#define SD_CARD_MIN_SIZE_MP4 0x1E00000

// Box Type
#define FTYP_T		CC("ftyp");
#define MP42_T		CC("mp42");
#define ISOM_T		CC("isom")
#define MDAT_T		CC("mdat")
#define MOOV_T		CC("moov")
#define MVHD_T		CC("mvhd")
#define TRAK_T		CC("trak")
#define EDTS_T		CC("edts")
#define ELST_T		CC("elst")
#define ESDS_T		CC("esds")
#define TKHD_T		CC("tkhd")
#define MDIA_T		CC("mdia")
#define MDHD_T		CC("mdhd")
#define HDLR_T		CC("hdlr")
#define MINF_T		CC("minf")
#define VMHD_T		CC("vmhd")
#define SMHD_T		CC("smhd")
#define DINF_T		CC("dinf")
#define DREF_T		CC("dref")
#define STBL_T		CC("stbl")
#define STSD_T		CC("stsd")
#define MJPG_T		CC("mjpg")
#define AVC1_T		CC("avc1")
#define AVCC_T		CC("avcC")
#define SOWT_T		CC("sowt")
#define MP4A_T		CC("mp4a")
#define STTS_T		CC("stts")
#define STSS_T		CC("stss")
#define STSC_T		CC("stsc")
#define STSZ_T		CC("stsz")
#define STCO_T		CC("stco")
#define UDTA_T		CC("udta")
#define VIDE_T		CC("vide")
#define SOUN_T		CC("soun")


#ifdef VISUAL_STUDIO_CODE
  #pragma pack(1)
#endif

typedef struct mp4_header_box_struct_h {
	uint32_t size;
	uint32_t type;
#ifdef VISUAL_STUDIO_CODE
} mp4_header_box_struct;
#else
} __attribute__((packed)) mp4_header_box_struct;
#endif

typedef struct ftyp_box_struct_h {
	mp4_header_box_struct ftyp_header;
	uint32_t  major_brand;
	uint32_t  minor_version;
	uint32_t  compatible_brands[3];
#ifdef VISUAL_STUDIO_CODE
} ftyp_box_struct;
#else
} __attribute__((packed)) ftyp_box_struct;
#endif

typedef struct mvhd_box_struct_h {
	mp4_header_box_struct mvhd_header;
	uint32_t version :8;
	uint32_t flags	 :24;
	uint32_t creation_time;
	uint32_t modification_time;
	uint32_t time_sczle;
	uint32_t duration;
	uint32_t rate;
	uint16_t volume;
	uint8_t  rev[10];
	uint32_t matrix[9];
	uint8_t  pre_defined[24];
        uint32_t next_track_id;
#ifdef VISUAL_STUDIO_CODE
} mvhd_box_struct;
#else
} __attribute__((packed)) mvhd_box_struct;
#endif

typedef struct tkhd_box_struct_h {
	mp4_header_box_struct tkhd_header;
	uint32_t version :8;
	uint32_t flags	 :24;
	uint32_t creation_time;
	uint32_t modification_time;
	uint32_t track_id;
	uint8_t  rev1[4];
	uint32_t duration;
	uint8_t  rev2[8];
	uint16_t layer;
	uint16_t alternate_group;
	uint16_t volume;
	uint8_t  rev3[2];
	uint32_t matrix[9];
	uint32_t width;
	uint32_t height;
#ifdef VISUAL_STUDIO_CODE
} tkhd_box_struct;
#else
} __attribute__((packed)) tkhd_box_struct;
#endif

typedef struct elst_box_struct_h {
	mp4_header_box_struct tkhd_header;
	uint32_t version :8;
	uint32_t flags	 :24;
	uint32_t entry_count;
	uint32_t duration;
	uint32_t start_time;
	uint32_t speed;
#ifdef VISUAL_STUDIO_CODE
} elst_box_struct;
#else
} __attribute__((packed)) elst_box_struct;
#endif

typedef struct mdhd_box_struct_h {
	mp4_header_box_struct mdhd_header;
	uint32_t version :8;
	uint32_t flags	 :24;
	uint32_t creation_time;
	uint32_t modification_time;
	uint32_t time_sczle;
	uint32_t duration;
	uint16_t language;
	uint8_t  pre_defined[2];
#ifdef VISUAL_STUDIO_CODE
} mdhd_box_struct;
#else
} __attribute__((packed)) mdhd_box_struct;
#endif

typedef struct hdlr_box_struct_h {
	mp4_header_box_struct hdlr_header;
	uint32_t version :8;
	uint32_t flags	 :24;
	uint8_t  pre_defined[4];
	uint32_t handler_type;
	uint8_t  rev[12];
	uint32_t name[3];
#ifdef VISUAL_STUDIO_CODE
} hdlr_box_struct;
#else
} __attribute__((packed)) hdlr_box_struct;
#endif

typedef struct vmhd_box_struct_h {
	mp4_header_box_struct vmhd_header;
	uint32_t version :8;
	uint32_t flags	 :24;
	uint16_t graphics_mode;
	uint16_t opcolor[3];
#ifdef VISUAL_STUDIO_CODE
} vmhd_box_struct;
#else
} __attribute__((packed)) vmhd_box_struct;
#endif

typedef struct smhd_box_struct_h {
	mp4_header_box_struct smhd_header;
	uint32_t version :8;
	uint32_t flags	 :24;
	uint16_t balance;
	uint8_t  rev[2];
#ifdef VISUAL_STUDIO_CODE
} smhd_box_struct;
#else
} __attribute__((packed)) smhd_box_struct;
#endif

typedef struct dref_box_struct_h {
	mp4_header_box_struct dref_header;
	uint32_t version :8;
	uint32_t flags	 :24;
	uint32_t entry_count;
	uint32_t data_entry[3];
#ifdef VISUAL_STUDIO_CODE
} dref_box_struct;
#else
} __attribute__((packed)) dref_box_struct;
#endif

typedef struct stsd_box_struct_h {
	mp4_header_box_struct stsd_header;
	uint32_t version :8;
	uint32_t flags	 :24;
	uint32_t entry_count;
	// visual_sample_entry or audio_sample_entry
#ifdef VISUAL_STUDIO_CODE
} stsd_box_struct;
#else
} __attribute__((packed)) stsd_box_struct;
#endif

typedef struct visual_sample_entry_struct_h {
	uint32_t size;
	uint32_t type;
	uint8_t  rev1[6];
	uint16_t index;
	uint8_t  pre_defined1[2];
	uint8_t  rev2[2];
	uint8_t  pre_defined2[12];
	uint16_t width;
	uint16_t height;
	uint32_t horizresolution;
	uint32_t vertresolution;
	uint8_t  rev3[4];
	uint16_t frame_count;
	uint8_t  compressorname[32];
	uint16_t depth;
	uint16_t pre_defined3;
#ifdef VISUAL_STUDIO_CODE
} visual_sample_entry_struct;
#else
} __attribute__((packed)) visual_sample_entry_struct;
#endif

typedef struct avcC_box_struct_h {
	uint32_t size;
	uint32_t type;
	uint8_t  version;
	uint8_t  profile_indication;
	uint8_t  profile_compatibility;
	uint8_t  level_indication;
	uint8_t  nalu_len;

/*	uint8_t  sps_cnt;
	uint16_t sps_len;
	char     *sps_buf;
	uint8_t  pps_cnt;
	uint16_t pps_len;
	char     *pps_buf;*/
#ifdef VISUAL_STUDIO_CODE
} avcC_box_struct;
#else
} __attribute__((packed)) avcC_box_struct;
#endif

typedef struct audio_sample_entry_struct_h {
	uint32_t size;
	uint32_t type;
	uint8_t  rev1[6];
	uint16_t index;
	uint8_t  rev2[8];
	uint16_t channelcount;
	uint16_t samplesize;
	uint8_t  pre_defined[2];
	uint8_t  rev3[2];
	uint32_t samplerate;
#ifdef VISUAL_STUDIO_CODE
} audio_sample_entry_struct;
#else
} __attribute__((packed)) audio_sample_entry_struct;
#endif

typedef struct esds_box_struct_h {
	uint32_t size;
	uint32_t type;
	uint32_t version :8;
	uint32_t flags	 :24;

	uint8_t  tag_ESDescr;		//03
	uint32_t size_ESDescr;		//80 80 80 22
	uint8_t  data_ESDescr[3];	//00 02 00

	uint8_t  tag_DecConfigDescr;		//04
	uint32_t size_DecConfigDescr;		//80 80 80 14
	uint8_t  data_DecConfigDescr[13];	//40 15 22 FF F6 00 7A 12 00 00 01 F4 25

	uint8_t  tag_DecSpecificDescr;		//05
	uint32_t size_DecSpecificDescr;		//80 80 80 02
	uint8_t  data_DecSpecificDescr[2];	//12 10

	uint8_t  tag_06;		//06
	uint32_t size_06;		//80 80 80 01
	uint8_t  data_06[1];	//02
#ifdef VISUAL_STUDIO_CODE
} esds_box_struct;
#else
} __attribute__((packed)) esds_box_struct;
#endif

typedef struct stts_box_struct_h {
	mp4_header_box_struct stts_header;
	uint32_t version :8;
	uint32_t flags	 :24;
	uint32_t entry_count;
#ifdef VISUAL_STUDIO_CODE
} stts_box_struct;
#else
} __attribute__((packed)) stts_box_struct;
#endif

typedef struct sample_duration_struct_h {
	uint32_t sample_count;
	uint32_t sample_duration;
#ifdef VISUAL_STUDIO_CODE
} sample_duration_struct;
#else
} __attribute__((packed)) sample_duration_struct;
#endif

typedef struct stss_box_struct_h {
	mp4_header_box_struct stss_header;
	uint32_t version :8;
	uint32_t flags	 :24;
	uint32_t entry_count;
#ifdef VISUAL_STUDIO_CODE
} stss_box_struct;
#else
} __attribute__((packed)) stss_box_struct;
#endif

typedef struct sample_sn_struct_h {
	uint32_t sn;
#ifdef VISUAL_STUDIO_CODE
} sample_sn_struct;
#else
} __attribute__((packed)) sample_sn_struct;
#endif

typedef struct stsc_box_struct_h {
	mp4_header_box_struct stsc_header;
	uint32_t version :8;
	uint32_t flags	 :24;
	uint32_t entry_count;
#ifdef VISUAL_STUDIO_CODE
} stsc_box_struct;
#else
} __attribute__((packed)) stsc_box_struct;
#endif

typedef struct sample_to_chunk_struct_h {
	uint32_t first_chunk;
	uint32_t samples_per_chunk;
	uint32_t sample_description_index;
#ifdef VISUAL_STUDIO_CODE
} sample_to_chunk_struct;
#else
} __attribute__((packed)) sample_to_chunk_struct;
#endif

typedef struct stsz_box_struct_h {
	mp4_header_box_struct stsz_header;
	uint32_t version :8;
	uint32_t flags	 :24;
	uint32_t sample_size_type;
	uint32_t entry_count;
#ifdef VISUAL_STUDIO_CODE
} stsz_box_struct;
#else
} __attribute__((packed)) stsz_box_struct;
#endif

typedef struct sample_size_struct_h {
	uint32_t size;
#ifdef VISUAL_STUDIO_CODE
} sample_size_struct;
#else
} __attribute__((packed)) sample_size_struct;
#endif

typedef struct stco_box_struct_h {
	mp4_header_box_struct stco_header;
	uint32_t version :8;
	uint32_t flags	 :24;
	uint32_t entry_count;
#ifdef VISUAL_STUDIO_CODE
} stco_box_struct;
#else
} __attribute__((packed)) stco_box_struct;
#endif

typedef struct chunk_offset_struct_h {
	uint32_t offset;
#ifdef VISUAL_STUDIO_CODE
} chunk_offset_struct;
#else
} __attribute__((packed)) chunk_offset_struct;
#endif

typedef struct udta_box_struct_h {
	mp4_header_box_struct udta_header;
	uint8_t  rev1[4];
	uint32_t mode;
	uint8_t  rev2[120];
#ifdef VISUAL_STUDIO_CODE
} udta_box_struct;
#else
} __attribute__((packed)) udta_box_struct;
#endif

#ifdef VISUAL_STUDIO_CODE
  #pragma pack()
#endif


int save_mp4_proc(char *path, char *vbuf, int vlen, char *abuf, int alen, int width, int height,
		int fps, int ip_frame, int enc_type, int step, unsigned long long *freesize, int play_mode, int *v_cnt, char *sps, int sps_len, char *pps, int pps_len, int a_delay_t, int a_src);
void Set_MP4_H264_Profile_Level(int profile, int level);
int check_freesize(unsigned long long *freesize, int w_size);
int check_freesize2(unsigned long long *freesize, int w_size);

void set_mdat_box(mp4_header_box_struct* mdat, uint32_t size);
void set_moov_box(mp4_header_box_struct* moov, uint32_t size);

#ifdef __cplusplus
}   // extern "C"
#endif

extern int audio_rate;

#endif	//__MP4_H__
