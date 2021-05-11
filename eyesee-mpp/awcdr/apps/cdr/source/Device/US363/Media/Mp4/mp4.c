/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Media/Mp4/mp4.h"

#include <stdio.h>
#include <stdint.h>
#include <malloc.h>

#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::Mp4"

int audio_rate = 44100;

ftyp_box_struct ftyp_box_s; 	//32byte
void set_ftyp_box(ftyp_box_struct *ftyp, int enc_type)
{
	ftyp->ftyp_header.size     = SWAP32(0x0000001C);       	// size
	ftyp->ftyp_header.type     = FTYP_T;		// ftyp
	ftyp->major_brand          = MP42_T; 		// isom
	ftyp->minor_version        = SWAP32(0x00000200); 	// isom version

	// compatible_brands[]
	ftyp->compatible_brands[0] = ISOM_T;		// "isom"
	if(enc_type == 0)
		ftyp->compatible_brands[1] = MJPG_T; 		// "mjpg"
	else
		ftyp->compatible_brands[1] = AVC1_T; 		// "avc1"
	ftyp->compatible_brands[2] = MP42_T;		// "mp42"
}
/*char ftyp_box[0x20] = { 0
	0x00, 0x00, 0x00, 0x20,                          	// size
	0x66, 0x74, 0x79, 0x70,                                 // ftyp
	0x69, 0x73, 0x6F, 0x6D,                                 // isom
	0x00, 0x00, 0x02, 0x00,                                 // isom version
//	0x69, 0x73, 0x6F, 0x6D, 0x6D, 0x6A, 0x70, 0x32          // isommjp2
//	0x69, 0x73, 0x6F, 0x6D, 0x6D, 0x6A, 0x70, 0x67          // isommjpg     // 說明遵守哪些協議(isom+mjpg)
	0x69, 0x73, 0x6F, 0x6D, 0x69, 0x73, 0x6F, 0x32, 0x6D, 0x6A, 0x70, 0x67, 0x6D, 0x70, 0x34, 0x31          // isomiso2mjpgmp41
};*/

mp4_header_box_struct mdat_header_box_s;	//8byte
void set_mdat_box(mp4_header_box_struct *mdat, uint32_t size)
{
	mdat->size = SWAP32(size); 	// size		***
	mdat->type = MDAT_T;        // mdat
}
/*char mdat_box[0x8] = {
	0x00, 0x00, 0x00, 0x00,		// size
	0x6D, 0x64, 0x61, 0x74          // mdat
};*/

mp4_header_box_struct moov_header_box_s;		//8byte
void set_moov_box(mp4_header_box_struct *moov, uint32_t size)
{
	moov->size = SWAP32(size);	// size		***
	moov->type = MOOV_T;        // moov
}
/*char moov_box[0x8] = {
	0x00, 0x00, 0x03, 0x79,		// size, {moov_box + mvhd_box + trak_box}
	0x6D, 0x6F, 0x6F, 0x76          // moov
};*/

mvhd_box_struct mvhd_box_s;				//108byte
void set_mvhd_box(mvhd_box_struct *mvhd, uint32_t c_time, uint32_t sczle, uint32_t duration, uint32_t n_track_id)
{
	mvhd->mvhd_header.size     = SWAP32(0x0000006C);
	mvhd->mvhd_header.type     = MVHD_T;
	mvhd->version          	   = 0;
	mvhd->flags		   		   = 0;
	mvhd->creation_time        = SWAP32(c_time);	  	// ***
	mvhd->modification_time    = SWAP32(c_time);   		// ***
	mvhd->time_sczle	       = SWAP32(sczle);
	mvhd->duration		       = SWAP32(duration); 		// ***
	mvhd->rate		           = SWAP32(0x00010000);
	mvhd->volume		       = SWAP16(0x0100);	//SWAP16(0x1000);
	//mvhd->rev
	memset(&mvhd->rev[0], 0, sizeof(uint8_t)*10);

	mvhd->matrix[0]		   = SWAP32(0x00010000);
	mvhd->matrix[1]		   = SWAP32(0x00000000);
	mvhd->matrix[2]		   = SWAP32(0x00000000);
	mvhd->matrix[3]		   = SWAP32(0x00000000);
	mvhd->matrix[4]		   = SWAP32(0x00010000);
	mvhd->matrix[5]		   = SWAP32(0x00000000);
	mvhd->matrix[6]		   = SWAP32(0x00000000);
	mvhd->matrix[7]		   = SWAP32(0x00000000);
	mvhd->matrix[8]		   = SWAP32(0x40000000);

	memset(&mvhd->pre_defined[0], 0, sizeof(mvhd->pre_defined) );
	mvhd->next_track_id  	   = SWAP32(n_track_id);	// ***
}
// 一個文件只能有一個 mvhd box
/*char mvhd_box[0x6C] = {
	0x00, 0x00, 0x00, 0x6C,         // size
	0x6D, 0x76, 0x68, 0x64,         // mvhd
	0x00,                           // version
	0x00, 0x00, 0x00,               // flags
	0x00, 0x00, 0x00, 0x00,         // creation time
	0x00, 0x00, 0x00, 0x00,         // modification time
	0x00, 0x00, 0x03, 0xE8,		// time sczle, 1000
	0x00, 0x00, 0x07, 0xD0,         // duration, 2000 = 2s
	0x00, 0x01, 0x00, 0x00,		// rate 			媒体速率，代表原始倍速
	0x01, 0x00,			// volume
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 	// rev

	// matrix
	0x00, 0x01, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x01, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x40, 0x00, 0x00, 0x00,

	// pre-defined
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	//0x00, 0x00, 0x00, 0x03		// next track id
	0x00, 0x00, 0x00, 0x02		// next track id
};*/

mp4_header_box_struct trak_header_box_s;		//8byte
void set_trak_box(mp4_header_box_struct *trak, uint32_t size)
{
	trak->size = SWAP32(size);	// size		***
	trak->type = TRAK_T;        // trak
}
// 軌道(視頻軌, 音軌, 字幕軌)
/*char trak_box[0x8] = {
	0x00, 0x00, 0x03, 0x05,		// size, {trak_box + tkhd_box + mdia_box}
	0x74, 0x72, 0x61, 0x6B          // trak
};*/

tkhd_box_struct tkhd_box_s;				//92byte
void set_tkhd_box(tkhd_box_struct *tkhd, uint32_t c_time, uint32_t track_id, uint32_t duration, uint32_t width, uint32_t height)
{
	int w, h;

	tkhd->tkhd_header.size     = SWAP32(0x0000005C);
	tkhd->tkhd_header.type     = TKHD_T;
	tkhd->version          	   = 0;
	tkhd->flags		   = SWAP24(0x000003);
	tkhd->creation_time        = SWAP32(c_time); 		// ***
	tkhd->modification_time    = SWAP32(c_time); 		// ***
	tkhd->track_id		   	   = SWAP32(track_id);		// ***
	memset(&tkhd->rev1[0], 0, sizeof(uint8_t)*4);
	tkhd->duration		       = SWAP32(duration);	  	// ***
	memset(&tkhd->rev2[0], 0, sizeof(uint8_t)*8);
	tkhd->layer		           = 0;
	if(track_id == 1) {
		tkhd->alternate_group	   = 0;
		tkhd->volume 		   = 0;
	}
	else {
		tkhd->alternate_group	   = SWAP16(0x0001);
		tkhd->volume 		   = SWAP16(0x0100);
	}
	memset(&tkhd->rev3[0], 0, sizeof(uint8_t)*2);

	tkhd->matrix[0]		   = SWAP32(0x00010000);
	tkhd->matrix[1]		   = SWAP32(0x00000000);
	tkhd->matrix[2]		   = SWAP32(0x00000000);
	tkhd->matrix[3]		   = SWAP32(0x00000000);
	tkhd->matrix[4]		   = SWAP32(0x00010000);
	tkhd->matrix[5]		   = SWAP32(0x00000000);
	tkhd->matrix[6]		   = SWAP32(0x00000000);
	tkhd->matrix[7]		   = SWAP32(0x00000000);
	tkhd->matrix[8]		   = SWAP32(0x40000000);

	if(track_id == 1) {
		w = (width & 0xFFFF) << 16;
		h = (height & 0xFFFF) << 16;
		tkhd->width		   = SWAP32(w);		// ***
		tkhd->height		   = SWAP32(h);		// ***
	}
	else {
		tkhd->width		   = SWAP32(0);		// ***
		tkhd->height		   = SWAP32(0);		// ***
	}
}
/*char tkhd_box[0x5C] = {
	0x00, 0x00, 0x00, 0x5C,		// size
	0x74, 0x6B, 0x68, 0x64,		// tkhd
	0x00,                           // version
	0x00, 0x00, 0x03,		// flags
	0x00, 0x00, 0x00, 0x00,         // creation time
	0x00, 0x00, 0x00, 0x00,         // modification time
	0x00, 0x00, 0x00, 0x01,		// track id   		非0且唯一
	0x00, 0x00, 0x00, 0x00,		// rev
	0x00, 0x00, 0x07, 0xD0,		// duration
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,		// rev
	0x00, 0x00,			// layer               	圖層, 數值越小越上層
	0x00, 0x00,			// alternate group  	備份ID, 播放時,相同ID只會播放一個
	0x00, 0x00,			// volume
	0x00, 0x00,			// rev

	// matrix
	0x00, 0x01, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x01, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x40, 0x00, 0x00, 0x00,

	0x05, 0x00, 0x00, 0x00,		// width
	//0x06, 0x00, 0x00, 0x00          // height
	0x02, 0xD0, 0x00, 0x00          // height
};*/

/*
tref box可以描述兩個track之間關係. (非必要, 暫時不加入)

ex: 一個mp4文件中有三條video track, ID分別是2 3 4, 以及三條audio track, ID分別是6 7 8

在播放track 2視頻時到底應該採用6 7 8哪條音頻與其配套播放? 這時候需要在track 2與6的tref box中指定一下, 將2與6兩條track綁定起來
*/

/*mp4_header_box_struct edts_header_box_s;		//8byte
void set_edts_box(mp4_header_box_struct *edts, uint32_t size)
{
	edts->size = SWAP32(size);	// size		***
	edts->type = EDTS_T;        // edts
}

elst_box_struct elst_box_s;				//28byte
void set_elst_box(elst_box_struct *elst, uint32_t duration)
{
	elst->tkhd_header.size     = SWAP32(0x0000001C);
	elst->tkhd_header.type     = ELST_T;
	elst->version          	   = 0;
	elst->flags		   		   = SWAP24(0x000000);
	elst->entry_count          = SWAP32(0x00000001);
	elst->duration			   = SWAP32(duration);
	elst->start_time		   = 0;						// 開始時間(偏移多少時間)
	elst->speed			       = SWAP32(0x00010000);	// 播放速度
}*/

mp4_header_box_struct mdia_header_box_s;	   	//8byte
void set_mdia_box(mp4_header_box_struct *mdia, uint32_t size)
{
	mdia->size = SWAP32(size);	// size		***
	mdia->type = MDIA_T;        // mdia
}
/*char mdia_box[0x8] = {
	0x00, 0x00, 0x02, 0xA1,		// size, {mdia_box + mdhd_box + hdlr_box + minf_box}
	0x6D, 0x64, 0x69, 0x61          // mdia
};*/

mdhd_box_struct mdhd_box_s;				//32byte
void set_mdhd_box(mdhd_box_struct *mdhd, uint32_t c_time, uint32_t sczle, uint32_t duration, int enc_type)
{
	mdhd->mdhd_header.size     = SWAP32(0x00000020);
	mdhd->mdhd_header.type     = MDHD_T;
	mdhd->version          	   = 0;
	mdhd->flags		   = 0;
	mdhd->creation_time        = SWAP32(c_time);	  	// ***
	mdhd->modification_time    = SWAP32(c_time);		// ***

	/*if(enc_type == 2) {
		mdhd->time_sczle	   = SWAP32(audio_rate);			// audio 44100
		mdhd->duration		   = SWAP32(duration);		// ***
	}
	else {*/
		mdhd->time_sczle	   = SWAP32(sczle);
		mdhd->duration		   = SWAP32(duration);		// ***
	//}

	mdhd->language		   = SWAP16(0x55C4);
	memset(&mdhd->pre_defined[0], 0, sizeof(mdhd->pre_defined) );
}
/*char mdhd_box[0x20] = {
	0x00, 0x00, 0x00, 0x20,		// size
	0x6D, 0x64, 0x68, 0x64,		// mdhd
	0x00,				// version
	0x00, 0x00, 0x00,		// flags
	0x00, 0x00, 0x00, 0x00,         // creation time
	0x00, 0x00, 0x00, 0x00,         // modification time
	0x00, 0x00, 0x03, 0xE8,		// time sczle, 1000
	0x00, 0x00, 0x07, 0xD0,         // duration, 2000 = 2s
	0x55, 0xC4,			// language
	0x00, 0x00,			// pre-defined
}; */

hdlr_box_struct hdlr_box_s;				//24byte
void set_hdlr_box(hdlr_box_struct *hdlr, int enc_type)
{
	hdlr->hdlr_header.size     = SWAP32(0x0000002C);
	hdlr->hdlr_header.type     = HDLR_T;
	hdlr->version          	   = 0;
	hdlr->flags		   = 0;
	memset(&hdlr->pre_defined[0], 0, sizeof(hdlr->pre_defined) );
	if(enc_type == 2) hdlr->handler_type = SOUN_T;	// ***
	else              hdlr->handler_type = VIDE_T;      // 0:mjpg 1:avc1 2:pcm
	//hdlr->rev
	memset(&hdlr->rev[0], 0, sizeof(uint8_t)*12);
	if(enc_type == 2) {		// SoundHandler
		hdlr->name[0] = CC("Soun");
		hdlr->name[1] = CC("dHan");
		hdlr->name[2] = CC("dler");
	}
	else {					// VideoHandler
		hdlr->name[0] = CC("Vide");
		hdlr->name[1] = CC("oHan");
		hdlr->name[2] = CC("dler");
	}
}
/*char hdlr_box[0x25] = {
	0x00, 0x00, 0x00, 0x25,	    	// size
	0x68, 0x64, 0x6C, 0x72,		// hdlr
	0x00,				// version
	0x00, 0x00, 0x00,		// flags
	0x00, 0x00, 0x00, 0x00,		// pre-defined
	0x76, 0x69, 0x64, 0x65,		// handler type, vide
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,		// rev
	0x76, 0x69, 0x64, 0x65, 0x6F	// name, video
};*/

mp4_header_box_struct minf_header_box_s;		//8byte
void set_minf_box(mp4_header_box_struct *minf, uint32_t size)
{
	minf->size = SWAP32(size);	// size		***
	minf->type = MINF_T;
}
/*char minf_box[0x8] = {
	0x00, 0x00, 0x02, 0x53,		// size, {minf_box + vmhd_box + dinf_box + stbl_box}
	0x6D, 0x69, 0x6E, 0x66          // minf
};*/

vmhd_box_struct vmhd_box_s; 				//20byte
void set_vmhd_box(vmhd_box_struct *vmhd)
{
	vmhd->vmhd_header.size     = SWAP32(0x00000014);
	vmhd->vmhd_header.type     = VMHD_T;
	vmhd->version          	   = 0;
	vmhd->flags		   = SWAP24(0x000001);
	vmhd->graphics_mode	   = 0;

	vmhd->opcolor[0]	   = 0;
	vmhd->opcolor[1]	   = 0;
	vmhd->opcolor[2]	   = 0;
}
/*char vmhd_box[0x14] = {
	0x00, 0x00, 0x00, 0x14,			// size
	0x76, 0x6D, 0x68, 0x64,			// vmhd
	0x00,					// version
	0x00, 0x00, 0x01,			// flags
	0x00, 0x00, 				// graphics mode
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// opcolor
};*/

smhd_box_struct smhd_box_s; 				//20byte
void set_smhd_box(smhd_box_struct *smhd)
{
	smhd->smhd_header.size = SWAP32(0x00000010);
	smhd->smhd_header.type = SMHD_T;
	smhd->version          = 0;
	smhd->flags		   	   = 0;
	smhd->balance	       = 0;
	memset(&smhd->rev[0], 0, sizeof(uint8_t)*2);
}
/*char smhd_box[0x14] = {
	0x00, 0x00, 0x00, 0x10,		// size
	0x73, 0x6D, 0x68, 0x64,		// vmhd
	0x00,						// version
	0x00, 0x00, 0x01,			// flags
	0x00, 0x00, 				// balance	左右聲道
	0x00, 0x00					// rev
};*/

mp4_header_box_struct dinf_header_box_s;		//8byte
void set_dinf_box(mp4_header_box_struct *dinf)
{
	dinf->size = SWAP32(0x00000024);
	dinf->type = DINF_T;
}
/*char dinf_box[0x8] = {
	0x00, 0x00, 0x00, 0x24,		// size, {dinf_box + dref_box}
	0x64, 0x69, 0x6E, 0x66          // dinf
};*/

dref_box_struct dref_box_s;				//28byte
void set_dref_box(dref_box_struct *dref)
{
	dref->dref_header.size     = SWAP32(0x0000001C);
	dref->dref_header.type     = DREF_T;
	dref->version          	   = 0;
	dref->flags		   = 0;
	dref->entry_count	   = SWAP32(0x00000001);

	dref->data_entry[0]	   = SWAP32(0x0000000C);
	dref->data_entry[1]	   = SWAP32(0x75726C20);
	dref->data_entry[2]	   = SWAP32(0x00000001);
}
/*char dref_box[0x1C] = {
	0x00, 0x00, 0x00, 0x1C,		// size
	0x64, 0x72, 0x65, 0x66,          // dref
	0x00,				// version
	0x00, 0x00, 0x00,		// flags
	0x00, 0x00, 0x00, 0x01,		// entry count

	// "url" or "urn"
	0x00, 0x00, 0x00, 0x0C,
	0x75, 0x72, 0x6C, 0x20,
	0x00, 0x00, 0x00, 0x01
};*/

mp4_header_box_struct stbl_header_box_s;		//8byte
void set_stbl_box(mp4_header_box_struct *stbl, uint32_t size)
{
	stbl->size = SWAP32(size);	// ***
	stbl->type = STBL_T;
}
/*char stbl_box[0x8] = {
	0x00, 0x00, 0x01, 0xCE,		// size, { stbl_box + stsd_box + stts_box + stss_box + stsc_box + stsz_box + stco_box}
	0x73, 0x74, 0x62, 0x6C          // stbl
};*/

stsd_box_struct stsd_box_s;				//16byte
//visual_sample_entry_struct visual_sample_entry;		//86byte 	**88byte
//audio_sample_entry_struct audio_sample_entry;		//36byte
void set_stsd_box(stsd_box_struct *stsd, uint32_t size, uint32_t e_cnt)
{
	stsd->stsd_header.size     = SWAP32(size);	     	// ***
	stsd->stsd_header.type     = STSD_T;
	stsd->version          	   = 0;
	stsd->flags		   = 0;
	stsd->entry_count	   = SWAP32(e_cnt);		// ***
}
void set_visual_sample_entry(visual_sample_entry_struct *entry, int enc_type, uint16_t index, uint16_t width, uint16_t height, int avcC_size)
{
	if(enc_type == 0)
		entry->size		   = SWAP32(0x00000056);
	else
		entry->size		   = SWAP32(0x00000056 + avcC_size);	// add v_stsd_tmp_buf[]
	if(enc_type == 0) entry->type = MJPG_T;		// ***
	else		  	  entry->type = AVC1_T;
	//entry->rev1
	memset(&entry->rev1[0], 0, sizeof(uint8_t)*6);
	entry->index		   = SWAP16(index);		// ***
	memset(&entry->pre_defined1[0], 0, sizeof(entry->pre_defined1) );
	//entry->rev2
	memset(&entry->rev2[0], 0, sizeof(uint8_t)*2);
	memset(&entry->pre_defined2[0], 0, sizeof(entry->pre_defined2) );
	entry->width		   = SWAP16(width);		// ***
	entry->height		   = SWAP16(height);		// ***
	entry->horizresolution	   = SWAP32(0x00480000);
	entry->vertresolution	   = SWAP32(0x00480000);
	//entry->rev3
	memset(&entry->rev3[0], 0, sizeof(uint8_t)*4);
	entry->frame_count	   = SWAP16(0x0001);
	memset(&entry->compressorname[0], 0, sizeof(entry->compressorname) );
	entry->depth		   = SWAP16(0x0018);
	entry->pre_defined3	   = SWAP16(0xFFFF);
}
int MP4_H264_Profile = 0x42;
int MP4_H264_Level   = 0x1E;
void Set_MP4_H264_Profile_Level(int profile, int level)
{
	MP4_H264_Profile = profile;
	MP4_H264_Level   = level;
}
void set_avcC_box(avcC_box_struct *avcC, int enc_type, int size, int profile, int level)
{
	avcC->size		  			 	= SWAP32(size);
	avcC->type 	   	   				= AVCC_T;		//avcC
	avcC->version					= 0x01;
	avcC->profile_indication		= profile;
	avcC->profile_compatibility		= 0x00;
	avcC->level_indication			= level;
	avcC->nalu_len					= 0xFF;
}
void set_audio_sample_entry(audio_sample_entry_struct *entry, uint16_t channel, uint16_t samplesize, uint32_t samplerate)
{
	uint32_t samplerate_tmp;

	//entry->size		    = SWAP32(0x00000057);	// 0x24 + esds(0x33) = 0x57
	entry->size		    = SWAP32(0x00000024);
    //entry->type         = MP4A_T;
    entry->type         = SOWT_T;
    memset(&entry->rev1[0], 0, sizeof(uint8_t)*6);
    entry->index        = SWAP16(0x0001);
    memset(&entry->rev2[0], 0, sizeof(uint8_t)*8);
    entry->channelcount = SWAP16(channel);
    entry->samplesize   = SWAP16(samplesize);
    memset(&entry->pre_defined[0], 0, sizeof(uint8_t)*2);
    memset(&entry->rev3[0], 0, sizeof(uint8_t)*2);

    samplerate_tmp = (samplerate & 0xFFFF) << 16;
    entry->samplerate   = SWAP32(samplerate_tmp);	// 44100
}
/*esds_box_struct esds_box_s;
void set_esds_box(esds_box_struct *esds)
{
	esds->size		  			 	= SWAP32(0x00000033);
	esds->type 	   	   				= ESDS_T;		//esds
	esds->version          	   		= 0;
	esds->flags		  			    = 0;

	esds->tag_ESDescr				= 0x03;
	esds->size_ESDescr				= SWAP32(0x80808022);
	esds->data_ESDescr[0]			= 0x00;	//00 02 00
	esds->data_ESDescr[1]			= 0x02;	//00 02 00
	esds->data_ESDescr[2]			= 0x00;	//00 02 00

	esds->tag_DecConfigDescr		= 0x04;		//04
	esds->size_DecConfigDescr		= SWAP32(0x80808014); //80 80 80 14
	esds->data_DecConfigDescr[0]	= 0x40;	//40 15 22 FF F6 00 7A 12 00 00 01 F4 25
	esds->data_DecConfigDescr[1]	= 0x15; //40 15 22 FF F6 00 7A 12 00 00 01 F4 25
	esds->data_DecConfigDescr[2]	= 0x22; //40 15 22 FF F6 00 7A 12 00 00 01 F4 25
	esds->data_DecConfigDescr[3]	= 0xFF; //40 15 22 FF F6 00 7A 12 00 00 01 F4 25
	esds->data_DecConfigDescr[4]	= 0xF6; //40 15 22 FF F6 00 7A 12 00 00 01 F4 25
	esds->data_DecConfigDescr[5]	= 0x00; //40 15 22 FF F6 00 7A 12 00 00 01 F4 25
	esds->data_DecConfigDescr[6]	= 0x7A; //40 15 22 FF F6 00 7A 12 00 00 01 F4 25
	esds->data_DecConfigDescr[7]	= 0x12; //40 15 22 FF F6 00 7A 12 00 00 01 F4 25
	esds->data_DecConfigDescr[8]	= 0x00; //40 15 22 FF F6 00 7A 12 00 00 01 F4 25
	esds->data_DecConfigDescr[9]	= 0x00; //40 15 22 FF F6 00 7A 12 00 00 01 F4 25
	esds->data_DecConfigDescr[10]	= 0x01; //40 15 22 FF F6 00 7A 12 00 00 01 F4 25
	esds->data_DecConfigDescr[11]	= 0xF4; //40 15 22 FF F6 00 7A 12 00 00 01 F4 25
	esds->data_DecConfigDescr[12]	= 0x25; //40 15 22 FF F6 00 7A 12 00 00 01 F4 25

	esds->tag_DecSpecificDescr		= 0x05;		//05
	esds->size_DecSpecificDescr    	= SWAP32(0x80808002);	//80 80 80 02
	esds->data_DecSpecificDescr[0]  = 0x12;	//12 10
	esds->data_DecSpecificDescr[1]  = 0x10;	//12 10

	esds->tag_06					= 0x06;		//06
	esds->size_06					= SWAP32(0x80808001);		//80 80 80 01
	esds->data_06[0] 				= 0x02;	//02
}*/
/*char stsd_box[0x6A] = {
	0x00, 0x00, 0x00, 0x6A,		// size
	0x73, 0x74, 0x73, 0x64,		// stsd
	0x00,				// version
	0x00, 0x00, 0x00,		// flags
	0x00, 0x00, 0x00, 0x01,		// entry count

	0x00, 0x00, 0x00, 0x56,		// size
//	0x6D, 0x6A, 0x70, 0x32,		// enc type, mjp2
	0x6D, 0x6A, 0x70, 0x67,		// enc type, mjpg
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// rev
	0x00, 0x01,   			// index
	0x00, 0x00,                    	// pre_defined
	0x00, 0x00,                   	// rev
	// pre_defined[]
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x05, 0x00,		       	// width
	//0x06, 0x00,			// height
	0x02, 0xD0,			// height
	0x00, 0x48, 0x00, 0x00,		// horizresolution
	0x00, 0x48, 0x00, 0x00,		// vertresolution
	0x00, 0x00, 0x00, 0x00, 	// rev
	0x00, 0x01,               	// frame_count = 1
	// compressorname[]
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x18,       		// depth = 0x0018
	0xFF, 0xFF,   			// pre_defined = -1
};*/


stts_box_struct stts_box_s;			//16byte
//sample_duration_struct sample_duration;		//8byte
void set_stts_box(stts_box_struct *stts, uint32_t size, uint32_t e_cnt)
{
	stts->stts_header.size     = SWAP32(size);	    	// ***
	stts->stts_header.type     = STTS_T;
	stts->version          	   = 0;
	stts->flags		   = 0;
	stts->entry_count	   = SWAP32(e_cnt);		// ***
}
void set_sample_duration(sample_duration_struct *sample, uint32_t cnt, uint32_t duration)
{
	sample->sample_count	   = SWAP32(cnt); 		// ***
	sample->sample_duration	   = SWAP32(duration);	       	// ***
}
/*char stts_box[0x18] = {
	0x00, 0x00, 0x00, 0x18,		// size
	0x73, 0x74, 0x74, 0x73,		// stts
	0x00,				// version
	0x00, 0x00, 0x00,		// flags
	0x00, 0x00, 0x00, 0x01,		// count

	0x00, 0x00, 0x00, 0x1E,		// sample count
	0x00, 0x00, 0x00, 0x42,		// sample duration, ???
};*/

stss_box_struct stss_box_s;			//16byte
//sample_sn_struct sample_sn;			//4byte
void set_stss_box(stss_box_struct *stss, uint32_t size, uint32_t e_cnt)
{
	stss->stss_header.size     = SWAP32(size);	       	// ***
	stss->stss_header.type     = STSS_T;
	stss->version          	   = 0;
	stss->flags		   = 0;
	stss->entry_count	   = SWAP32(e_cnt);		// ***
}
void set_sample_sn(sample_sn_struct *sample, uint32_t sn)
{
	sample->sn	   	   = SWAP32(sn); 		// ***
}
/*char stss_box[0x14] = {
	0x00, 0x00, 0x00, 0x14,		// size
	0x73, 0x74, 0x73, 0x73,		// stss
	0x00,				// version
	0x00, 0x00, 0x00,		// flags
	0x00, 0x00, 0x00, 0x01,		// count

	// 16 byte
	// I-frame sample SN
	0x00, 0x00, 0x00, 0x01		// sample SN, I-frame sample 序號
};*/

stsc_box_struct stsc_box_s;			//16byte
//sample_to_chunk_struct sample_to_chunk;		//12byte
void set_stsc_box(stsc_box_struct *stsc, uint32_t size, uint32_t e_cnt)
{
	stsc->stsc_header.size     = SWAP32(size);		// ***
	stsc->stsc_header.type     = STSC_T;
	stsc->version          	   = 0;
	stsc->flags		   = 0;
	stsc->entry_count	   = SWAP32(e_cnt);		// ***
}
void set_sample_to_chunk(sample_to_chunk_struct *sample, uint32_t first, uint32_t s_cnt, uint32_t index)
{
	sample->first_chunk	   = SWAP32(first); 		// ***
	sample->samples_per_chunk  = SWAP32(s_cnt);		// ***
	sample->sample_description_index = SWAP32(index);       // ***
}
/*char stsc_box[0x1C] = {
	0x00, 0x00, 0x00, 0x1C,		// size
	0x73, 0x74, 0x73, 0x63,		// stsc
	0x00,				// version
	0x00, 0x00, 0x00,		// flags
	0x00, 0x00, 0x00, 0x01,		// count

	// 16 byte
	// chunk
	0x00, 0x00, 0x00, 0x01,		// first chunk[]
	0x00, 0x00, 0x00, 0x01,        	// samples per chunk[]
	0x00, 0x00, 0x00, 0x01          // sample description index[]
};*/

stsz_box_struct stsz_box_s;			//20byte
//sample_size_struct sample_size; 		//4byte
void set_stsz_box(stsz_box_struct *stsz, uint32_t size, uint32_t e_cnt)
{
	stsz->stsz_header.size     = SWAP32(size);		// ***
	stsz->stsz_header.type     = STSZ_T;
	stsz->version          	   = 0;
	stsz->flags		   = 0;
	stsz->sample_size_type     = 0;
	stsz->entry_count	   = SWAP32(e_cnt);		// ***
}
void set_sample_size(sample_size_struct *sample, uint32_t size)
{
	sample->size	   	   = SWAP32(size); 		// ***
}
/*char stsz_box[0x8C] = {
	0x00, 0x00, 0x00, 0x8C,		// size
	0x73, 0x74, 0x73, 0x7A,		// stsz
	0x00,				// version
	0x00, 0x00, 0x00,		// flags
	0x00, 0x00, 0x00, 0x00,		// sample size type
	0x00, 0x00, 0x00, 0x1E,		// count

	// 20 byte
	// sample size[0x1E]
};*/

stco_box_struct stco_box_s;			//16byte
//chunk_offset_struct chunk_offset; 		//4byte
void set_stco_box(stco_box_struct *stco, uint32_t size, uint32_t e_cnt)
{
	stco->stco_header.size     = SWAP32(size);	// ***
	stco->stco_header.type     = STCO_T;
	stco->version          	   = 0;
	stco->flags		   = 0;
	stco->entry_count	   = SWAP32(e_cnt);	// ***
}
void set_chunk_offset(chunk_offset_struct *sample, uint32_t offset)
{
	sample->offset	   	   = SWAP32(offset); 		// ***
}
/*char stco_box[0x88] = {
	0x00, 0x00, 0x00, 0x88, 	// size
	0x73, 0x74, 0x63, 0x6F,		// stco
	0x00,				// version
	0x00, 0x00, 0x00,		// flags
	0x00, 0x00, 0x00, 0x1E,		// count

	// 16 byte
	// chunk offset size[0x1E]
};*/

udta_box_struct udta_box_s;
void set_udta_box(udta_box_struct *udta, int mode)
{
	udta->udta_header.size     = SWAP32(0x00000088);
	udta->udta_header.type     = UDTA_T;
	memset(&udta->rev1[0], 0, sizeof(uint8_t)*4);
	switch(mode) {
	case 0: udta->mode = CC(ALETA_MODE_GLOBAL_STR); break;
	case 1: udta->mode = CC(ALETA_MODE_FRONT_STR);  break;
	case 2: udta->mode = CC(ALETA_MODE_360_STR);    break;
	case 3: break;
	case 4: udta->mode = CC(ALETA_MODE_180_STR);    break;
	case 5: udta->mode = CC(ALETA_MODE_4SPLIT_STR); break;
	case 6: break;
	}
	memset(&udta->rev2[0], 0, sizeof(uint8_t)*120);
}

char Metadata_buf[0x1D4] = {
 0x00, 0x00, 0x01, 0xD4, 0x75, 0x75, 0x69, 0x64, 0xFF, 0xCC, 0x82, 0x63, 0xF8, 0x55, 0x4A, 0x93,
 0x88, 0x14, 0x58, 0x7A, 0x02, 0x52, 0x1F, 0xDD, 0x3C, 0x3F, 0x78, 0x6D, 0x6C, 0x20, 0x76, 0x65,
 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x3D, 0x22, 0x31, 0x2E, 0x30, 0x22, 0x3F, 0x3E, 0x0A, 0x3C, 0x72,
 0x64, 0x66, 0x3A, 0x53, 0x70, 0x68, 0x65, 0x72, 0x69, 0x63, 0x61, 0x6C, 0x56, 0x69, 0x64, 0x65,
 0x6F, 0x20, 0x78, 0x6D, 0x6C, 0x6E, 0x73, 0x3A, 0x72, 0x64, 0x66, 0x3D, 0x22, 0x68, 0x74, 0x74,
 0x70, 0x3A, 0x2F, 0x2F, 0x77, 0x77, 0x77, 0x2E, 0x77, 0x33, 0x2E, 0x6F, 0x72, 0x67, 0x2F, 0x31,
 0x39, 0x39, 0x39, 0x2F, 0x30, 0x32, 0x2F, 0x32, 0x32, 0x2D, 0x72, 0x64, 0x66, 0x2D, 0x73, 0x79,
 0x6E, 0x74, 0x61, 0x78, 0x2D, 0x6E, 0x73, 0x23, 0x22, 0x20, 0x78, 0x6D, 0x6C, 0x6E, 0x73, 0x3A,
 0x47, 0x53, 0x70, 0x68, 0x65, 0x72, 0x69, 0x63, 0x61, 0x6C, 0x3D, 0x22, 0x68, 0x74, 0x74, 0x70,
 0x3A, 0x2F, 0x2F, 0x6E, 0x73, 0x2E, 0x67, 0x6F, 0x6F, 0x67, 0x6C, 0x65, 0x2E, 0x63, 0x6F, 0x6D,
 0x2F, 0x76, 0x69, 0x64, 0x65, 0x6F, 0x73, 0x2F, 0x31, 0x2E, 0x30, 0x2F, 0x73, 0x70, 0x68, 0x65,
 0x72, 0x69, 0x63, 0x61, 0x6C, 0x2F, 0x22, 0x3E, 0x0A, 0x20, 0x20, 0x3C, 0x47, 0x53, 0x70, 0x68,
 0x65, 0x72, 0x69, 0x63, 0x61, 0x6C, 0x3A, 0x53, 0x70, 0x68, 0x65, 0x72, 0x69, 0x63, 0x61, 0x6C,
 0x3E, 0x74, 0x72, 0x75, 0x65, 0x3C, 0x2F, 0x47, 0x53, 0x70, 0x68, 0x65, 0x72, 0x69, 0x63, 0x61,
 0x6C, 0x3A, 0x53, 0x70, 0x68, 0x65, 0x72, 0x69, 0x63, 0x61, 0x6C, 0x3E, 0x0A, 0x20, 0x20, 0x3C,
 0x47, 0x53, 0x70, 0x68, 0x65, 0x72, 0x69, 0x63, 0x61, 0x6C, 0x3A, 0x53, 0x74, 0x69, 0x74, 0x63,
 0x68, 0x65, 0x64, 0x3E, 0x74, 0x72, 0x75, 0x65, 0x3C, 0x2F, 0x47, 0x53, 0x70, 0x68, 0x65, 0x72,
 0x69, 0x63, 0x61, 0x6C, 0x3A, 0x53, 0x74, 0x69, 0x74, 0x63, 0x68, 0x65, 0x64, 0x3E, 0x0A, 0x20,
 0x20, 0x3C, 0x47, 0x53, 0x70, 0x68, 0x65, 0x72, 0x69, 0x63, 0x61, 0x6C, 0x3A, 0x53, 0x74, 0x69,
 0x74, 0x63, 0x68, 0x69, 0x6E, 0x67, 0x53, 0x6F, 0x66, 0x74, 0x77, 0x61, 0x72, 0x65, 0x3E, 0x53,
 0x70, 0x68, 0x65, 0x72, 0x69, 0x63, 0x61, 0x6C, 0x20, 0x4D, 0x65, 0x74, 0x61, 0x64, 0x61, 0x74,
 0x61, 0x20, 0x54, 0x6F, 0x6F, 0x6C, 0x3C, 0x2F, 0x47, 0x53, 0x70, 0x68, 0x65, 0x72, 0x69, 0x63,
 0x61, 0x6C, 0x3A, 0x53, 0x74, 0x69, 0x74, 0x63, 0x68, 0x69, 0x6E, 0x67, 0x53, 0x6F, 0x66, 0x74,
 0x77, 0x61, 0x72, 0x65, 0x3E, 0x0A, 0x20, 0x20, 0x3C, 0x47, 0x53, 0x70, 0x68, 0x65, 0x72, 0x69,
 0x63, 0x61, 0x6C, 0x3A, 0x50, 0x72, 0x6F, 0x6A, 0x65, 0x63, 0x74, 0x69, 0x6F, 0x6E, 0x54, 0x79,
 0x70, 0x65, 0x3E, 0x65, 0x71, 0x75, 0x69, 0x72, 0x65, 0x63, 0x74, 0x61, 0x6E, 0x67, 0x75, 0x6C,
 0x61, 0x72, 0x3C, 0x2F, 0x47, 0x53, 0x70, 0x68, 0x65, 0x72, 0x69, 0x63, 0x61, 0x6C, 0x3A, 0x50,
 0x72, 0x6F, 0x6A, 0x65, 0x63, 0x74, 0x69, 0x6F, 0x6E, 0x54, 0x79, 0x70, 0x65, 0x3E, 0x0A, 0x3C,
 0x2F, 0x72, 0x64, 0x66, 0x3A, 0x53, 0x70, 0x68, 0x65, 0x72, 0x69, 0x63, 0x61, 0x6C, 0x56, 0x69,
 0x64, 0x65, 0x6F, 0x3E
};

/*
ftyp
mdat
...
moov
	mvhd
	trak
		tkhd
		mdia
			mdhd
			hdlr
			minf
				vmhd
				dinf
					dref
				stbl
					stsd
					stts
					stss
					stsc
					stsz
					stco
		metadata
	trak
		tkhd
		mdia
			mdhd
			hdlr
			minf
				smhd
				dinf
					dref
				stbl
					stsd
					stts
					stss
					stsc
					stsz
					stco
udta
*/

FILE *mp4_fp=NULL;
char mp4_header_buf[MP4_HEADER_BUF_SIZE];	// 1kb
char v_header_buf[MP4_HEADER_BUF_SIZE];		// 1kb
char a_header_buf[MP4_HEADER_BUF_SIZE];		// 1kb

char v_stts_e_buf[STTS_E_BUF_SIZE];			// 430kb, 30fps * 1800sec * 8byte
char v_stss_e_buf[STSS_E_BUF_SIZE];			// 220kb, 30fps * 1800sec * 4byte
char v_stsc_e_buf[STSC_E_BUF_SIZE];			// 850kb, 30fps * 1800sec * 16byte
char v_stsz_e_buf[STSZ_E_BUF_SIZE];     	// 220kb, 30fps * 1800sec * 4byte
char v_stco_e_buf[STCO_E_BUF_SIZE];     	// 220kb, 30fps * 1800sec * 4byte

char a_stts_e_buf[STTS_E_BUF_SIZE];			// 430kb, 30fps * 1800sec * 8byte
char a_stsc_e_buf[STSC_E_BUF_SIZE];			// 850kb, 30fps * 1800sec * 16byte
char a_stsz_e_buf[STSZ_E_BUF_SIZE];     	// 220kb, 30fps * 1800sec * 4byte
char a_stco_e_buf[STCO_E_BUF_SIZE];     	// 220kb, 30fps * 1800sec * 4byte

/*int write_video_to_file(FILE *fp, char *buf, int len, int *mdat_s, int *stts_c, int *stsc_e_c, char *stsc_e_p, int *stss_e_c, char *stss_e_p,
		int *stco_e_c, char *stco_e_p, int *stco_o, int *stsz_e_c, char *stsz_e_p, sample_size_struct *sample_size_lst, chunk_offset_struct *chunk_offset_lst)
{
	int stsc_first, stsc_sample_cnt, stsc_index;
	int stss_sn;
	int stsz_sample_size;

	sample_to_chunk_struct     sample_to_chunk;
	sample_sn_struct           sample_sn;
	sample_size_struct         sample_size;
	chunk_offset_struct        chunk_offset;

	if(buf != NULL) {
		fwrite(&buf[0], len, 1, fp);
		*mdat_s += len;
		*stts_c = *stts_c + 1;

		//if(ip_frame == 1) {
			*stsc_e_c = *stsc_e_c + 1;
			stsc_first      = *stsc_e_c;
			stsc_sample_cnt = 1; //v_stts_cnt - v_stss_sn;
			stsc_index      = 1;
			set_sample_to_chunk(&sample_to_chunk, stsc_first, stsc_sample_cnt, stsc_index);
			memcpy(stsc_e_p, &sample_to_chunk, sizeof(sample_to_chunk) );
			stsc_e_p += sizeof(sample_to_chunk);

			*stss_e_c = *stss_e_c + 1;
			stss_sn = *stts_c;
			set_sample_sn(&sample_sn, stss_sn);
			memcpy(stss_e_p, &sample_sn, sizeof(sample_sn) );
			stss_e_p += sizeof(sample_sn);

			*stco_e_c = *stco_e_c + 1;
			set_chunk_offset(&chunk_offset, *stco_o);
			memcpy(stco_e_p, &chunk_offset, sizeof(chunk_offset) );
			stco_e_p += sizeof(chunk_offset);
			*stco_o += len;
			memcpy(&chunk_offset_lst, &chunk_offset, sizeof(chunk_offset) );
			//chunk_offset_lst = chunk_offset;
		//}

		*stsz_e_c = *stsz_e_c + 1;
		stsz_sample_size = len;
		set_sample_size(&sample_size, stsz_sample_size);
		memcpy(stsz_e_p, &sample_size, sizeof(sample_size) );
		stsz_e_p += sizeof(sample_size);
		memcpy(&sample_size_lst, &sample_size, sizeof(sample_size) );
		//sample_size_lst = sample_size;
	}
	else {
		*stco_e_c = *stco_e_c + 1;
		memcpy(stco_e_p, &sample_size_lst, sizeof(sample_size_lst) );
		stco_e_p += sizeof(sample_size_lst);

		*stsz_e_c = *stsz_e_c + 1;
		memcpy(stsz_e_p, &sample_size, sizeof(sample_size) );
		stsz_e_p += sizeof(sample_size);
	}

	return 0;
}*/

/*int write_audio_to_file(FILE *fp, char *buf, int len, int *mdat_s, int *stts_c, int *stsc_e_c, char *stsc_e_p,
		int *stco_e_c, char *stco_e_p, int *stco_o, int *stsz_e_c, char *stsz_e_p)
{
	int stsc_first, stsc_sample_cnt, stsc_index;
	int stsz_sample_size;

	sample_to_chunk_struct     sample_to_chunk;
	sample_size_struct         sample_size;
	chunk_offset_struct        chunk_offset;

	if(buf != NULL) {
		fwrite(&buf[0], len, 1, fp);
		*mdat_s += len;
		*stts_c++;

		//if(ip_frame == 1) {
			*stsc_e_c = *stsc_e_c + 1;
			stsc_first      = *stsc_e_c;
			stsc_sample_cnt = 1;
			stsc_index      = 1;
			set_sample_to_chunk(&sample_to_chunk, stsc_first, stsc_sample_cnt, stsc_index);
			memcpy(stsc_e_p, &sample_to_chunk, sizeof(sample_to_chunk) );
			stsc_e_p += sizeof(sample_to_chunk);

			*stco_e_c = *stco_e_c + 1;
			set_chunk_offset(&chunk_offset, *stco_o);
			memcpy(stco_e_p, &chunk_offset, sizeof(chunk_offset) );
			stco_e_p += sizeof(chunk_offset);
			*stco_o += len;
		//}

		*stsz_e_c = *stsz_e_c + 1;
		stsz_sample_size = len;
		set_sample_size(&sample_size, stsz_sample_size);
		memcpy(stsz_e_p, &sample_size, sizeof(sample_size) );
		stsz_e_p += sizeof(sample_size);
	}

	return 0;
}*/

int check_freesize(unsigned long long *freesize, int w_size)
{
	if(*freesize > w_size) {
		*freesize -= w_size;
		if(*freesize < SD_CARD_MIN_SIZE_MP4)
			return -1;
	}
	else return -1;

	return 0;
}

int check_freesize2(unsigned long long *freesize, int w_size)
{
	if(*freesize > w_size)
		*freesize -= w_size;
	else
		return -2;

	return 0;
}

int save_mp4_proc(char *path, char *vbuf, int vlen, char *abuf, int alen, int width, int height,
		int fps, int ip_frame, int enc_type, int step, unsigned long long *freesize, int play_mode,
		int *v_cnt, char *sps, int sps_len, char *pps, int pps_len, int a_delay_t, int a_src)
{
	int i, ret, mp4_fd;
	unsigned long long freesize_tmp;

	int avcC_size;
	uint8_t sps_cnt, pps_cnt;
	uint16_t sps_len_t, pps_len_t;

	static int jump_cnt = 0;	// 跳張調整stts, 延長上一張播放時間

	static int mdat_size=0, moov_size=0; 
	static int v_trak_size=0, v_edts_size=0, v_mdia_size=0, v_minf_size=0;
	static int v_stbl_size=0, v_stsd_size=0, v_stts_size=0, v_stss_size=0, v_stsc_size=0, v_stsz_size=0, v_stco_size=0;
	
	static int a_trak_size=0, a_edts_size=0, a_mdia_size=0, a_minf_size=0;
	static int a_stbl_size=0, a_stsd_size=0, a_stts_size=0, a_stsc_size=0, a_stsz_size=0, a_stco_size=0;
	
	static int c_time=0, duration=0, a_duration=0, track_id=1, n_track_id=2, duration_cnt=0;
	
	static int v_stsd_e_cnt=0, v_stts_e_cnt=1, v_stss_e_cnt=0, v_stsc_e_cnt=0, v_stsz_e_cnt=0, v_stco_e_cnt=0;
	static int v_stts_cnt=0, v_stts_duration=0;
	static int v_stss_sn=0;
	static int v_stsc_first=0, v_stsc_sample_cnt=0, v_stsc_index=1;
	static int v_stsz_sample_size=0;
	static int stco_offset=0;
	
	static int a_stsd_e_cnt=0, a_stts_e_cnt=0, a_stsc_e_cnt=0, a_stsz_e_cnt=0, a_stco_e_cnt=0;
	static int a_stts_cnt=0, a_stts_duration=0;
	static int a_stsc_first=0, a_stsc_sample_cnt=0, a_stsc_index=1;
	static int a_stsz_sample_size=0;

	static char *header_ptr, *v_header_ptr, *a_header_ptr;
	static fpos_t mdatPos;
	static char *moovPos, *mvhdPos, *mvhdPos_dur; 
	static char *v_trakPos, *v_tkhdPos, *v_tkhdPos_dur, *v_edtsPos, *v_elstPos, *v_elstPos_dur, *v_mdiaPos, *v_mdhdPos, *v_mdhdPos_dur, *v_minfPos, *v_stblPos, *v_stsdPos;
	static char *a_trakPos, *a_tkhdPos, *a_tkhdPos_dur, *a_edtsPos, *a_elstPos, *a_elstPos_dur, *a_mdiaPos, *a_mdhdPos, *a_mdhdPos_dur, *a_minfPos, *a_stblPos, *a_stsdPos;

	static char *v_stts_e_ptr, *v_stts_s_ptr, *v_stts_c_ptr;
	static char *v_stss_e_ptr, *v_stss_s_ptr, *v_stss_c_ptr;
	static char *v_stsc_e_ptr, *v_stsc_s_ptr, *v_stsc_c_ptr;
	static char *v_stsz_e_ptr, *v_stsz_s_ptr, *v_stsz_c_ptr;
	static char *v_stco_e_ptr, *v_stco_s_ptr, *v_stco_c_ptr;
	static char *v_stts_e_ptr_lst;

	static char *a_stts_e_ptr, *a_stts_s_ptr, *a_stts_c_ptr;
	static char *a_stsc_e_ptr, *a_stsc_s_ptr, *a_stsc_c_ptr;
	static char *a_stsz_e_ptr, *a_stsz_s_ptr, *a_stsz_c_ptr;
	static char *a_stco_e_ptr, *a_stco_s_ptr, *a_stco_c_ptr;

	visual_sample_entry_struct visual_sample_entry;
	audio_sample_entry_struct  audio_sample_entry;
	sample_duration_struct     sample_duration;
	sample_sn_struct           sample_sn;
	sample_to_chunk_struct     sample_to_chunk;
	sample_size_struct         sample_size;
	chunk_offset_struct        chunk_offset;
	static sample_size_struct  v_sample_size_lst;
	static chunk_offset_struct v_chunk_offset_lst;
	avcC_box_struct			   avcC_box;

	freesize_tmp = *freesize;

	//setLEDLightOn(1, 7);
	//setLEDLightOn(0, 7);
#ifdef ANDROID_CODE
//tmp	setLEDLightOfDataSize(0, vlen + alen);
#endif
	switch(step) {
	case 0:		//init
		mdat_size=0; moov_size=0; 
		v_trak_size=0; v_edts_size=0, v_mdia_size=0; v_minf_size=0;
		v_stbl_size=0; v_stsd_size=0; v_stts_size=0; v_stss_size=0; v_stsc_size=0; v_stsz_size=0; v_stco_size=0;

		c_time=0; duration=0; a_duration=0; track_id=1; n_track_id=2; duration_cnt=0;
		
		v_stsd_e_cnt=0; v_stts_e_cnt=0; v_stss_e_cnt=0; v_stsc_e_cnt=0; v_stsz_e_cnt=0; v_stco_e_cnt=0;
		v_stts_cnt=0; v_stts_duration=0;
		v_stss_sn =0;
		v_stsc_first=0; v_stsc_sample_cnt=0; v_stsc_index=1;
		v_stsz_sample_size=0;
		stco_offset=0;
		
		a_stsd_e_cnt=0; a_stts_e_cnt=0; a_stsc_e_cnt=0; a_stsz_e_cnt=0; a_stco_e_cnt=0;
		a_stts_cnt=0; a_stts_duration=0;
		a_stsc_first=0; a_stsc_sample_cnt=0; a_stsc_index=1;
		a_stsz_sample_size=0;

//tmp		mdatPos=NULL;

		if(mp4_fp != NULL)   fclose(mp4_fp);
		mp4_fp = fopen(path, "wb");
		if(mp4_fp == NULL) return -1;


		memset(&mp4_header_buf[0], 0, MP4_HEADER_BUF_SIZE);
		memset(  &v_header_buf[0], 0, MP4_HEADER_BUF_SIZE);
		memset(  &a_header_buf[0], 0, MP4_HEADER_BUF_SIZE);

		memset(&v_stts_e_buf[0], 0, STTS_E_BUF_SIZE);
		memset(&v_stss_e_buf[0], 0, STSS_E_BUF_SIZE);
		memset(&v_stsc_e_buf[0], 0, STSC_E_BUF_SIZE);
		memset(&v_stsz_e_buf[0], 0, STSZ_E_BUF_SIZE);
		memset(&v_stco_e_buf[0], 0, STCO_E_BUF_SIZE);

		memset(&a_stts_e_buf[0], 0, STTS_E_BUF_SIZE);
		memset(&a_stsc_e_buf[0], 0, STSC_E_BUF_SIZE);
		memset(&a_stsz_e_buf[0], 0, STSZ_E_BUF_SIZE);
		memset(&a_stco_e_buf[0], 0, STCO_E_BUF_SIZE);


		header_ptr   = &mp4_header_buf[0];
		v_header_ptr = &v_header_buf[0];
		a_header_ptr = &a_header_buf[0];

		v_stts_e_ptr = &v_stts_e_buf[0] + sizeof(stts_box_s);
		v_stss_e_ptr = &v_stss_e_buf[0] + sizeof(stss_box_s);
		v_stsc_e_ptr = &v_stsc_e_buf[0] + sizeof(stsc_box_s);
		v_stsz_e_ptr = &v_stsz_e_buf[0] + sizeof(stsz_box_s);
		v_stco_e_ptr = &v_stco_e_buf[0] + sizeof(stco_box_s);
		v_stts_e_ptr_lst = v_stts_e_ptr;

		a_stts_e_ptr = &a_stts_e_buf[0] + sizeof(stts_box_s);
		a_stsc_e_ptr = &a_stsc_e_buf[0] + sizeof(stsc_box_s);
		a_stsz_e_ptr = &a_stsz_e_buf[0] + sizeof(stsz_box_s);
		a_stco_e_ptr = &a_stco_e_buf[0] + sizeof(stco_box_s);

		// ftyp + mdat BOX ====================================================
		set_ftyp_box(&ftyp_box_s, enc_type);
		ret = fwrite(&ftyp_box_s, sizeof(ftyp_box_s), 1, mp4_fp);
		if(ret == 0) return -4;
		stco_offset += sizeof(ftyp_box_s);

		fgetpos(mp4_fp, &mdatPos);
		set_mdat_box(&mdat_header_box_s, mdat_size);
		ret = fwrite(&mdat_header_box_s, sizeof(mdat_header_box_s), 1, mp4_fp);
		if(ret == 0) return -4;
		stco_offset += sizeof(mdat_header_box_s);

		// video moov ~ stsd BOX =====================================================
		moovPos = header_ptr;
		set_moov_box(&moov_header_box_s, moov_size);
		memcpy(header_ptr, &moov_header_box_s, sizeof(moov_header_box_s) );
		header_ptr += sizeof(moov_header_box_s);

		mvhdPos = header_ptr;
		c_time = 0; 
		if(vbuf != NULL && abuf != NULL) n_track_id = 3;
		else if(audio_rate == -1)		 n_track_id = 2;
		else				 			 n_track_id = 2;	//縮時
		set_mvhd_box(&mvhd_box_s, c_time, audio_rate, duration, n_track_id);
		memcpy(header_ptr, &mvhd_box_s, sizeof(mvhd_box_s) );
		header_ptr += sizeof(mvhd_box_s);

		
		v_trakPos = v_header_ptr;
		set_trak_box(&trak_header_box_s, v_trak_size);
		memcpy(v_header_ptr, &trak_header_box_s, sizeof(trak_header_box_s) );
		v_header_ptr += sizeof(trak_header_box_s);

			v_tkhdPos = v_header_ptr;
			track_id = 1;
			set_tkhd_box(&tkhd_box_s, c_time, track_id, duration, width, height);
			memcpy(v_header_ptr, &tkhd_box_s, sizeof(tkhd_box_s) );
			v_header_ptr += sizeof(tkhd_box_s);

/*			v_edts_size = 0x24;
			v_edtsPos = v_header_ptr;
			set_edts_box(&edts_header_box_s, v_edts_size);
			memcpy(v_header_ptr, &edts_header_box_s, sizeof(edts_header_box_s) );
			v_header_ptr += sizeof(edts_header_box_s);

				v_elstPos = v_header_ptr;
				set_elst_box(&elst_box_s, duration);
				memcpy(v_header_ptr, &elst_box_s, sizeof(elst_box_s) );
				v_header_ptr += sizeof(elst_box_s);*/

			v_mdiaPos = v_header_ptr;
			set_mdia_box(&mdia_header_box_s, v_mdia_size);
			memcpy(v_header_ptr, &mdia_header_box_s, sizeof(mdia_header_box_s) );
			v_header_ptr += sizeof(mdia_header_box_s);

				v_mdhdPos = v_header_ptr;
				set_mdhd_box(&mdhd_box_s, c_time, audio_rate, duration, enc_type);
				memcpy(v_header_ptr, &mdhd_box_s, sizeof(mdhd_box_s) );
				v_header_ptr += sizeof(mdhd_box_s);

				set_hdlr_box(&hdlr_box_s, enc_type);
				memcpy(v_header_ptr, &hdlr_box_s, sizeof(hdlr_box_s) );
				v_header_ptr += sizeof(hdlr_box_s);

				v_minfPos = v_header_ptr;
				set_minf_box(&minf_header_box_s, v_minf_size);
				memcpy(v_header_ptr, &minf_header_box_s, sizeof(minf_header_box_s) );
				v_header_ptr += sizeof(minf_header_box_s);

					set_vmhd_box(&vmhd_box_s);
					memcpy(v_header_ptr, &vmhd_box_s, sizeof(vmhd_box_s) );
					v_header_ptr += sizeof(vmhd_box_s);

					set_dinf_box(&dinf_header_box_s);
					memcpy(v_header_ptr, &dinf_header_box_s, sizeof(dinf_header_box_s) );
					v_header_ptr += sizeof(dinf_header_box_s);

						set_dref_box(&dref_box_s);
						memcpy(v_header_ptr, &dref_box_s, sizeof(dref_box_s) );
						v_header_ptr += sizeof(dref_box_s);

					v_stblPos = v_header_ptr;
					set_stbl_box(&stbl_header_box_s, v_stbl_size);
					memcpy(v_header_ptr, &stbl_header_box_s, sizeof(stbl_header_box_s) );
					v_header_ptr += sizeof(stbl_header_box_s);

						avcC_size = sizeof(avcC_box) + sps_len + pps_len + 6;		// +6: sps_cnt(1Byte) + sps_len(2Byte) + pps_cnt(1Byte) + pps_len(2Byte)
						v_stsdPos = v_header_ptr;
						v_stsd_e_cnt = 1;
						if(enc_type == 0)
							v_stsd_size = v_stsd_e_cnt * sizeof(visual_sample_entry) + sizeof(stsd_box_s); //v_stsd_size = v_stsd_e_cnt * 86 + sizeof(stsd_box_s); //
						else
							v_stsd_size = v_stsd_e_cnt * sizeof(visual_sample_entry) + sizeof(stsd_box_s) + avcC_size;
						set_stsd_box(&stsd_box_s, v_stsd_size, v_stsd_e_cnt);
						memcpy(v_header_ptr, &stsd_box_s, sizeof(stsd_box_s) );
						v_header_ptr += sizeof(stsd_box_s);
						//for(i = 0; i < v_stsd_e_cnt; i++) {
							set_visual_sample_entry(&visual_sample_entry, enc_type, v_stsd_e_cnt, width & 0xFFFF, height & 0xFFFF, avcC_size);
							memcpy(v_header_ptr, &visual_sample_entry, sizeof(visual_sample_entry) ); //memcpy(v_header_ptr, &visual_sample_entry, 86 ); //
							v_header_ptr += sizeof(visual_sample_entry); //v_header_ptr += 86; //
							if(enc_type == 1) {
								set_avcC_box(&avcC_box, enc_type, avcC_size, MP4_H264_Profile, MP4_H264_Level);
								memcpy(v_header_ptr, &avcC_box, sizeof(avcC_box) );
								v_header_ptr += sizeof(avcC_box);

								sps_cnt = 1 | 0xE0;
								sps_len_t = SWAP16(sps_len & 0xFFFF);
								pps_cnt = 1;
								pps_len_t = SWAP16(pps_len & 0xFFFF);
								memcpy(v_header_ptr, &sps_cnt, sizeof(sps_cnt) );
								v_header_ptr += sizeof(sps_cnt);
								memcpy(v_header_ptr, &sps_len_t, sizeof(sps_len_t) );
								v_header_ptr += sizeof(sps_len_t);
								memcpy(v_header_ptr, &sps[0], sps_len);
								v_header_ptr += sps_len;

								memcpy(v_header_ptr, &pps_cnt, sizeof(pps_cnt) );
								v_header_ptr += sizeof(pps_cnt);
								memcpy(v_header_ptr, &pps_len_t, sizeof(pps_len_t) );
								v_header_ptr += sizeof(pps_len_t);
								memcpy(v_header_ptr, &pps[0], pps_len);
								v_header_ptr += pps_len;
							}
						//}

		// audio moov ~ stsd BOX =====================================================
		if(n_track_id == 3) {				
			a_trakPos = a_header_ptr;
			set_trak_box(&trak_header_box_s, a_trak_size);
			memcpy(a_header_ptr, &trak_header_box_s, sizeof(trak_header_box_s) );
			a_header_ptr += sizeof(trak_header_box_s);

				a_tkhdPos = a_header_ptr;
				track_id = 2;
				set_tkhd_box(&tkhd_box_s, c_time, track_id, duration, width, height);
				memcpy(a_header_ptr, &tkhd_box_s, sizeof(tkhd_box_s) );
				a_header_ptr += sizeof(tkhd_box_s);

/*				a_edts_size = 0x24;
				a_edtsPos = a_header_ptr;
				set_edts_box(&edts_header_box_s, a_edts_size);
				memcpy(a_header_ptr, &edts_header_box_s, sizeof(edts_header_box_s) );
				a_header_ptr += sizeof(edts_header_box_s);

					a_elstPos = a_header_ptr;
					set_elst_box(&elst_box_s, duration);
					memcpy(a_header_ptr, &elst_box_s, sizeof(elst_box_s) );
					a_header_ptr += sizeof(elst_box_s);*/

				a_mdiaPos = a_header_ptr;
				set_mdia_box(&mdia_header_box_s, a_mdia_size);
				memcpy(a_header_ptr, &mdia_header_box_s, sizeof(mdia_header_box_s) );
				a_header_ptr += sizeof(mdia_header_box_s);

					a_mdhdPos = a_header_ptr;
					set_mdhd_box(&mdhd_box_s, c_time, audio_rate, a_duration, 2);
					memcpy(a_header_ptr, &mdhd_box_s, sizeof(mdhd_box_s) );
					a_header_ptr += sizeof(mdhd_box_s);

					set_hdlr_box(&hdlr_box_s, 2);
					memcpy(a_header_ptr, &hdlr_box_s, sizeof(hdlr_box_s) );
					a_header_ptr += sizeof(hdlr_box_s);

					a_minfPos = a_header_ptr;
					set_minf_box(&minf_header_box_s, a_minf_size);
					memcpy(a_header_ptr, &minf_header_box_s, sizeof(minf_header_box_s) );
					a_header_ptr += sizeof(minf_header_box_s);

						set_smhd_box(&smhd_box_s);
						memcpy(a_header_ptr, &smhd_box_s, sizeof(smhd_box_s) );
						a_header_ptr += sizeof(smhd_box_s);

						set_dinf_box(&dinf_header_box_s);
						memcpy(a_header_ptr, &dinf_header_box_s, sizeof(dinf_header_box_s) );
						a_header_ptr += sizeof(dinf_header_box_s);

							set_dref_box(&dref_box_s);
							memcpy(a_header_ptr, &dref_box_s, sizeof(dref_box_s) );
							a_header_ptr += sizeof(dref_box_s);

						a_stblPos = a_header_ptr;
						set_stbl_box(&stbl_header_box_s, a_stbl_size);
						memcpy(a_header_ptr, &stbl_header_box_s, sizeof(stbl_header_box_s) );
						a_header_ptr += sizeof(stbl_header_box_s);

							a_stsdPos = a_header_ptr;
							a_stsd_e_cnt = 1;
							a_stsd_size = a_stsd_e_cnt * (sizeof(audio_sample_entry) ) + sizeof(stsd_box_s)/* + sizeof(esds_box_s)*/;
							set_stsd_box(&stsd_box_s, a_stsd_size, a_stsd_e_cnt);
							memcpy(a_header_ptr, &stsd_box_s, sizeof(stsd_box_s) );
							a_header_ptr += sizeof(stsd_box_s);
							//for(i = 0; i < a_stsd_e_cnt; i++) {
								set_audio_sample_entry(&audio_sample_entry, 1, 16, audio_rate);
								memcpy(a_header_ptr, &audio_sample_entry, sizeof(audio_sample_entry) );
								a_header_ptr += sizeof(audio_sample_entry);
							//}

/*							set_esds_box(&esds_box_s);
							memcpy(a_header_ptr, &esds_box_s, sizeof(esds_box_s) );
							a_header_ptr += sizeof(esds_box_s);*/
		}

		// write data =====================================================
		if(vbuf != NULL) {
			if(check_freesize(freesize, vlen) != 0) return -2;

			ret = fwrite(&vbuf[0], vlen, 1, mp4_fp);
			if(ret == 0) return -4;
			mdat_size += vlen;
			v_stts_e_cnt++;
			duration_cnt++;

			v_stsc_e_cnt++;
			v_stsc_first      = v_stsc_e_cnt;
			v_stsc_sample_cnt = 1;
			v_stsc_index      = 1;
			set_sample_to_chunk(&sample_to_chunk, v_stsc_first, v_stsc_sample_cnt, v_stsc_index);
			memcpy(v_stsc_e_ptr, &sample_to_chunk, sizeof(sample_to_chunk) );
			v_stsc_e_ptr += sizeof(sample_to_chunk);

			if(ip_frame == 1) {
				v_stss_e_cnt++;
				v_stss_sn = v_stts_e_cnt;
				set_sample_sn(&sample_sn, v_stss_sn);
				memcpy(v_stss_e_ptr, &sample_sn, sizeof(sample_sn) );
				v_stss_e_ptr += sizeof(sample_sn);
			}

			v_stco_e_cnt++;
			set_chunk_offset(&chunk_offset, stco_offset);
			memcpy(v_stco_e_ptr, &chunk_offset, sizeof(chunk_offset) );
			v_stco_e_ptr += sizeof(chunk_offset);
			stco_offset += vlen;
			v_chunk_offset_lst = chunk_offset;

			v_stsz_e_cnt++;
			v_stsz_sample_size = vlen;
			set_sample_size(&sample_size, v_stsz_sample_size);
			memcpy(v_stsz_e_ptr, &sample_size, sizeof(sample_size) );
			v_stsz_e_ptr += sizeof(sample_size);
			v_sample_size_lst = sample_size;

			v_stts_cnt = 1;
			v_stts_duration = audio_rate / (fps / 10);
			jump_cnt = 0;
			set_sample_duration(&sample_duration, v_stts_cnt, v_stts_duration);
			memcpy(v_stts_e_ptr, &sample_duration, sizeof(sample_duration) );
			v_stts_e_ptr_lst = v_stts_e_ptr;
			v_stts_e_ptr += sizeof(sample_duration);
		}
		else {
			duration_cnt++;
			jump_cnt++;
			v_stts_cnt = 1;
			v_stts_duration = audio_rate * (jump_cnt+1) / (fps / 10);
			//jump_cnt = 0;
			set_sample_duration(&sample_duration, v_stts_cnt, v_stts_duration);
			memcpy(v_stts_e_ptr_lst, &sample_duration, sizeof(sample_duration) );
		}

		if(abuf != NULL) {
			int delay_cnt = 1;
			char *delay_buf = malloc(alen);
			delay_cnt = (a_delay_t * fps / 10000) + 1;
			//if(a_src == 1) delay_cnt = (a_delay_t * fps / 10000) + 1;		//usb mic delay 1s
			//else		   delay_cnt = 1;
			if (delay_buf != NULL) {
				memset(&delay_buf[0], 0, alen);
				for (i = 0; i < delay_cnt; i++) {
					if (check_freesize(freesize, alen) != 0) return -1;

					if (i == (delay_cnt - 1)) ret = fwrite(&abuf[0], alen, 1, mp4_fp);
					else					  ret = fwrite(&delay_buf[0], alen, 1, mp4_fp);
					if (ret == 0) return -4;
					mdat_size += alen;
					a_stts_cnt++;

					if (i == 0) {
						a_stsc_e_cnt++;
						a_stsc_first      = a_stsc_e_cnt;
						a_stsc_sample_cnt = 1;
						a_stsc_index      = 1;
						set_sample_to_chunk(&sample_to_chunk, a_stsc_first, a_stsc_sample_cnt, a_stsc_index);
							memcpy(a_stsc_e_ptr, &sample_to_chunk, sizeof(sample_to_chunk));
						a_stsc_e_ptr += sizeof(sample_to_chunk);
					}

					a_stco_e_cnt++;
					set_chunk_offset(&chunk_offset, stco_offset);
					memcpy(a_stco_e_ptr, &chunk_offset, sizeof(chunk_offset));
					a_stco_e_ptr += sizeof(chunk_offset);
					stco_offset += alen;

					a_stsz_e_cnt++;
					a_stsz_sample_size = alen;
					set_sample_size(&sample_size, a_stsz_sample_size);
					memcpy(a_stsz_e_ptr, &sample_size, sizeof(sample_size));
					a_stsz_e_ptr += sizeof(sample_size);
				}
				free(delay_buf);
			}
		}

		break;
	case 1:		//ing
		if(mp4_fp == NULL) return -4;
		// write data =====================================================
		if(vbuf != NULL) {
			if(check_freesize(freesize, vlen) != 0) return -2;

			ret = fwrite(&vbuf[0], vlen, 1, mp4_fp);
			if(ret == 0) return -4;
			mdat_size += vlen;
			v_stts_e_cnt++;
			duration_cnt++;

			/*v_stsc_e_cnt++;
			v_stsc_first      = v_stsc_e_cnt;
			v_stsc_sample_cnt = 1;
			v_stsc_index      = 1;
			set_sample_to_chunk(&sample_to_chunk, v_stsc_first, v_stsc_sample_cnt, v_stsc_index);
			memcpy(v_stsc_e_ptr, &sample_to_chunk, sizeof(sample_to_chunk) );
			v_stsc_e_ptr += sizeof(sample_to_chunk);*/

			if(ip_frame == 1) {
				v_stss_e_cnt++;
				v_stss_sn = v_stts_e_cnt;
				set_sample_sn(&sample_sn, v_stss_sn);
				memcpy(v_stss_e_ptr, &sample_sn, sizeof(sample_sn) );
				v_stss_e_ptr += sizeof(sample_sn);
			}

			v_stco_e_cnt++;
			set_chunk_offset(&chunk_offset, stco_offset);
			memcpy(v_stco_e_ptr, &chunk_offset, sizeof(chunk_offset) );
			v_stco_e_ptr += sizeof(chunk_offset);
			stco_offset += vlen;
			v_chunk_offset_lst = chunk_offset;

			v_stsz_e_cnt++;
			v_stsz_sample_size = vlen;
			set_sample_size(&sample_size, v_stsz_sample_size);
			memcpy(v_stsz_e_ptr, &sample_size, sizeof(sample_size) );
			v_stsz_e_ptr += sizeof(sample_size);
			v_sample_size_lst = sample_size;

			v_stts_cnt = 1;
			v_stts_duration = audio_rate / (fps / 10);
			jump_cnt = 0;
			set_sample_duration(&sample_duration, v_stts_cnt, v_stts_duration);
			memcpy(v_stts_e_ptr, &sample_duration, sizeof(sample_duration) );
			v_stts_e_ptr_lst = v_stts_e_ptr;
			v_stts_e_ptr += sizeof(sample_duration);
		}
		else {
			duration_cnt++;
			jump_cnt++;
			v_stts_cnt = 1;
			v_stts_duration = audio_rate * (jump_cnt+1) / (fps / 10);
			//jump_cnt = 0;
			set_sample_duration(&sample_duration, v_stts_cnt, v_stts_duration);
			memcpy(v_stts_e_ptr_lst, &sample_duration, sizeof(sample_duration) );
		}

		if(abuf != NULL) {
			if(check_freesize(freesize, alen) != 0) return -1;

			ret = fwrite(&abuf[0], alen, 1, mp4_fp);
			if(ret == 0) return -4;
			mdat_size += alen;
			a_stts_cnt++;

			/*a_stsc_e_cnt++;
			a_stsc_first      = a_stsc_e_cnt;
			a_stsc_sample_cnt = 1;
			a_stsc_index      = 1;
			set_sample_to_chunk(&sample_to_chunk, a_stsc_first, a_stsc_sample_cnt, a_stsc_index);
			memcpy(a_stsc_e_ptr, &sample_to_chunk, sizeof(sample_to_chunk) );
			a_stsc_e_ptr += sizeof(sample_to_chunk);*/

			a_stco_e_cnt++;
			set_chunk_offset(&chunk_offset, stco_offset);
			memcpy(a_stco_e_ptr, &chunk_offset, sizeof(chunk_offset) );
			a_stco_e_ptr += sizeof(chunk_offset);
			stco_offset += alen;

			a_stsz_e_cnt++;
			a_stsz_sample_size = alen;
			set_sample_size(&sample_size, a_stsz_sample_size);
			memcpy(a_stsz_e_ptr, &sample_size, sizeof(sample_size) );
			a_stsz_e_ptr += sizeof(sample_size);
		}

		if(mdat_size > REC_PER_FILE_SIZE_MP4) return -3;

		*v_cnt = *v_cnt + 1;

		break;
	case -1:      	//end
	case -3:
		if(mp4_fp == NULL) return -4;

		if(header_ptr == NULL)   header_ptr   = &mp4_header_buf[0];
		if(v_header_ptr == NULL) v_header_ptr = &v_header_buf[0];
		if(a_header_ptr == NULL) a_header_ptr = &a_header_buf[0];

		if(v_stts_e_ptr == NULL) v_stts_e_ptr = &v_stts_e_buf[0] + sizeof(stts_box_s);
		if(v_stss_e_ptr == NULL) v_stss_e_ptr = &v_stss_e_buf[0] + sizeof(stss_box_s);
		if(v_stsc_e_ptr == NULL) v_stsc_e_ptr = &v_stsc_e_buf[0] + sizeof(stsc_box_s);
		if(v_stsz_e_ptr == NULL) v_stsz_e_ptr = &v_stsz_e_buf[0] + sizeof(stsz_box_s);
		if(v_stco_e_ptr == NULL) v_stco_e_ptr = &v_stco_e_buf[0] + sizeof(stco_box_s);
		if(v_stts_e_ptr_lst == NULL) v_stts_e_ptr_lst = v_stts_e_ptr;

		if(a_stts_e_ptr == NULL) a_stts_e_ptr = &a_stts_e_buf[0] + sizeof(stts_box_s);
		if(a_stsc_e_ptr == NULL) a_stsc_e_ptr = &a_stsc_e_buf[0] + sizeof(stsc_box_s);
		if(a_stsz_e_ptr == NULL) a_stsz_e_ptr = &a_stsz_e_buf[0] + sizeof(stsz_box_s);
		if(a_stco_e_ptr == NULL) a_stco_e_ptr = &a_stco_e_buf[0] + sizeof(stco_box_s);

		// write data =====================================================
		if(vbuf != NULL) {
			//if(check_freesize(freesize, vlen) != 0) return -2;		// weber-170109, 容量滿導致最後一個檔案無法播放(結尾檔頭未寫)
			if(check_freesize2(freesize, vlen) != 0) return -2;
			ret = fwrite(&vbuf[0], vlen, 1, mp4_fp);
			if(ret == 0) return -4;
			mdat_size += vlen;
			v_stts_e_cnt++;
			duration_cnt++;

			/*v_stsc_e_cnt++;
			v_stsc_first      = v_stsc_e_cnt;
			v_stsc_sample_cnt = 1;
			v_stsc_index      = 1;
			set_sample_to_chunk(&sample_to_chunk, v_stsc_first, v_stsc_sample_cnt, v_stsc_index);
			memcpy(v_stsc_e_ptr, &sample_to_chunk, sizeof(sample_to_chunk) );
			v_stsc_e_ptr += sizeof(sample_to_chunk);*/

			if(ip_frame == 1) {
				v_stss_e_cnt++;
				v_stss_sn = v_stts_e_cnt;
				set_sample_sn(&sample_sn, v_stss_sn);
				memcpy(v_stss_e_ptr, &sample_sn, sizeof(sample_sn) );
				v_stss_e_ptr += sizeof(sample_sn);
			}

			v_stco_e_cnt++;
			set_chunk_offset(&chunk_offset, stco_offset);
			memcpy(v_stco_e_ptr, &chunk_offset, sizeof(chunk_offset) );
			v_stco_e_ptr += sizeof(chunk_offset);
			stco_offset += vlen;
			v_chunk_offset_lst = chunk_offset;

			v_stsz_e_cnt++;
			v_stsz_sample_size = vlen;
			set_sample_size(&sample_size, v_stsz_sample_size);
			memcpy(v_stsz_e_ptr, &sample_size, sizeof(sample_size) );
			v_stsz_e_ptr += sizeof(sample_size);
			v_sample_size_lst = sample_size;

			v_stts_cnt = 1;
			v_stts_duration = audio_rate / (fps / 10);
			jump_cnt = 0;
			set_sample_duration(&sample_duration, v_stts_cnt, v_stts_duration);
			memcpy(v_stts_e_ptr, &sample_duration, sizeof(sample_duration) );
			v_stts_e_ptr_lst = v_stts_e_ptr;
			v_stts_e_ptr += sizeof(sample_duration);
		}
		else {
			duration_cnt++;
			jump_cnt++;
			v_stts_cnt = 1;
			v_stts_duration = audio_rate * (jump_cnt+1) / (fps / 10);
			//jump_cnt = 0;
			set_sample_duration(&sample_duration, v_stts_cnt, v_stts_duration);
			memcpy(v_stts_e_ptr_lst, &sample_duration, sizeof(sample_duration) );
		}

		if(abuf != NULL) {
			//if(check_freesize(freesize, alen) != 0) return -1;		// weber-170109, 容量滿導致最後一個檔案無法播放(結尾檔頭未寫)
			if(check_freesize2(freesize, alen) != 0) return -1;
			ret = fwrite(&abuf[0], alen, 1, mp4_fp);
			if(ret == 0) return -4;
			mdat_size += alen;
			a_stts_cnt++;

			/*a_stsc_e_cnt++;
			a_stsc_first      = a_stsc_e_cnt;
			a_stsc_sample_cnt = 1;
			a_stsc_index      = 1;
			set_sample_to_chunk(&sample_to_chunk, a_stsc_first, a_stsc_sample_cnt, a_stsc_index);
			memcpy(a_stsc_e_ptr, &sample_to_chunk, sizeof(sample_to_chunk) );
			a_stsc_e_ptr += sizeof(sample_to_chunk);*/

			a_stco_e_cnt++;
			set_chunk_offset(&chunk_offset, stco_offset);
			memcpy(a_stco_e_ptr, &chunk_offset, sizeof(chunk_offset) );
			a_stco_e_ptr += sizeof(chunk_offset);
			stco_offset += alen;

			a_stsz_e_cnt++;
			a_stsz_sample_size = alen;
			set_sample_size(&sample_size, a_stsz_sample_size);
			memcpy(a_stsz_e_ptr, &sample_size, sizeof(sample_size) );
			a_stsz_e_ptr += sizeof(sample_size);
		}

// 填檔頭 ====================================================================================
		v_stts_size = v_stts_e_cnt * sizeof(sample_duration) + sizeof(stts_box_s);
		set_stts_box(&stts_box_s, v_stts_size, v_stts_e_cnt);
		memcpy(&v_stts_e_buf[0], &stts_box_s, sizeof(stts_box_s) );

		v_stss_size = v_stss_e_cnt * sizeof(sample_sn) + sizeof(stss_box_s);
		set_stss_box(&stss_box_s, v_stss_size, v_stss_e_cnt);
		memcpy(&v_stss_e_buf[0], &stss_box_s, sizeof(stss_box_s) );

		v_stsc_size = v_stsc_e_cnt * sizeof(sample_to_chunk) + sizeof(stsc_box_s);
		set_stsc_box(&stsc_box_s, v_stsc_size, v_stsc_e_cnt);
		memcpy(&v_stsc_e_buf[0], &stsc_box_s, sizeof(stsc_box_s) );

		v_stsz_size = v_stsz_e_cnt * sizeof(sample_size) + sizeof(stsz_box_s);
		set_stsz_box(&stsz_box_s, v_stsz_size, v_stsz_e_cnt);
		memcpy(&v_stsz_e_buf[0], &stsz_box_s, sizeof(stsz_box_s) );

		v_stco_size = v_stco_e_cnt * sizeof(chunk_offset) + sizeof(stco_box_s);
		set_stco_box(&stco_box_s, v_stco_size, v_stco_e_cnt);
		memcpy(&v_stco_e_buf[0], &stco_box_s, sizeof(stco_box_s) );

		v_stbl_size = v_stsd_size + v_stts_size + v_stss_size + v_stsc_size + v_stsz_size + v_stco_size + sizeof(stbl_header_box_s);
		v_minf_size = sizeof(vmhd_box_s) + sizeof(dinf_header_box_s) + sizeof(dref_box_s) + v_stbl_size + sizeof(minf_header_box_s);
		v_mdia_size = sizeof(mdhd_box_s) + sizeof(hdlr_box_s) + v_minf_size + sizeof(mdia_header_box_s);
//		if(play_mode == 0 || play_mode == 2) {
//			if(width == 6144)	//max+ 170125 11M && 7M 無底部黑色, 所以不加環景檔頭
//				v_trak_size = sizeof(tkhd_box_s) /*+ v_edts_size*/ + v_mdia_size + sizeof(trak_header_box_s);
//			else
				v_trak_size = sizeof(tkhd_box_s) /*+ v_edts_size*/ + v_mdia_size + sizeof(trak_header_box_s) + sizeof(Metadata_buf);
//		}
//		else
//			v_trak_size = sizeof(tkhd_box_s) /*+ v_edts_size*/ + v_mdia_size + sizeof(trak_header_box_s);


		if(abuf != NULL) {
			a_stts_e_cnt = 1;
			a_stts_size = a_stts_e_cnt * sizeof(sample_duration) + sizeof(stts_box_s);
			set_stts_box(&stts_box_s, a_stts_size, a_stts_e_cnt);
			memcpy(&a_stts_e_buf[0], &stts_box_s, sizeof(stts_box_s) );
			for(i = 0; i < a_stts_e_cnt; i++) {
				a_stts_duration = audio_rate / (fps / 10);
				set_sample_duration(&sample_duration, a_stts_cnt, a_stts_duration);
				memcpy(a_stts_e_ptr, &sample_duration, sizeof(sample_duration) );
				a_stts_e_ptr += sizeof(sample_duration);
			}

			a_stsc_size = a_stsc_e_cnt * sizeof(sample_to_chunk) + sizeof(stsc_box_s);
			set_stsc_box(&stsc_box_s, a_stsc_size, a_stsc_e_cnt);
			memcpy(&a_stsc_e_buf[0], &stsc_box_s, sizeof(stsc_box_s) );

			a_stsz_size = a_stsz_e_cnt * sizeof(sample_size) + sizeof(stsz_box_s);
			set_stsz_box(&stsz_box_s, a_stsz_size, a_stsz_e_cnt);
			memcpy(&a_stsz_e_buf[0], &stsz_box_s, sizeof(stsz_box_s) );

			a_stco_size = a_stco_e_cnt * sizeof(chunk_offset) + sizeof(stco_box_s);
			set_stco_box(&stco_box_s, a_stco_size, a_stco_e_cnt);
			memcpy(&a_stco_e_buf[0], &stco_box_s, sizeof(stco_box_s) );

			a_stbl_size = a_stsd_size + a_stts_size + a_stsc_size + a_stsz_size + a_stco_size + sizeof(stbl_header_box_s);
			a_minf_size = sizeof(smhd_box_s) + sizeof(dinf_header_box_s) + sizeof(dref_box_s) + a_stbl_size + sizeof(minf_header_box_s);
			a_mdia_size = sizeof(mdhd_box_s) + sizeof(hdlr_box_s) + a_minf_size + sizeof(mdia_header_box_s);
			a_trak_size = sizeof(tkhd_box_s) /*+ a_edts_size*/ + a_mdia_size + sizeof(trak_header_box_s);
		}

		if(abuf != NULL)
			moov_size = sizeof(mvhd_box_s) + v_trak_size + sizeof(moov_header_box_s) + a_trak_size;
		else
			moov_size = sizeof(mvhd_box_s) + v_trak_size + sizeof(moov_header_box_s);


		//duration = v_stts_e_cnt * v_stts_duration;
		duration = duration_cnt * audio_rate / (fps / 10);
		//duration = v_stts_e_cnt * 1000 / (fps / 10);
		a_duration = duration/* * audio_rate / audio_rate*/;
//db_debug("mp4() duration=%d v_stts_e_cnt=%d fps=%d a_duration=%d alen=%d a_stts_cnt=%d\n", duration, v_stts_e_cnt, fps, a_duration, alen, a_stts_cnt);
		duration = SWAP32(duration);
		a_duration = SWAP32(a_duration);


		v_stbl_size = SWAP32(v_stbl_size);
		memcpy(v_stblPos, &v_stbl_size, sizeof(v_stbl_size) );

		v_minf_size = SWAP32(v_minf_size);
		memcpy(v_minfPos, &v_minf_size, sizeof(v_minf_size) );

		v_mdhdPos_dur = v_mdhdPos + 24;
		memcpy(v_mdhdPos_dur, &duration, sizeof(duration) );

		v_mdia_size = SWAP32(v_mdia_size);
		memcpy(v_mdiaPos, &v_mdia_size, sizeof(v_mdia_size) );

/*		v_elstPos_dur = v_elstPos + 16;
		memcpy(v_elstPos_dur, &duration, sizeof(duration) );*/

		v_tkhdPos_dur = v_tkhdPos + 28;
		memcpy(v_tkhdPos_dur, &duration, sizeof(duration) );

		v_trak_size = SWAP32(v_trak_size);
		memcpy(v_trakPos, &v_trak_size, sizeof(v_trak_size) );

		if(abuf != NULL) {
			a_stbl_size = SWAP32(a_stbl_size);
			memcpy(a_stblPos, &a_stbl_size, sizeof(a_stbl_size) );

			a_minf_size = SWAP32(a_minf_size);
			memcpy(a_minfPos, &a_minf_size, sizeof(a_minf_size) );

			a_mdhdPos_dur = a_mdhdPos + 24;
			memcpy(a_mdhdPos_dur, &a_duration, sizeof(duration) );

			a_mdia_size = SWAP32(a_mdia_size);
			memcpy(a_mdiaPos, &a_mdia_size, sizeof(a_mdia_size) );

/*			a_elstPos_dur = a_elstPos + 16;
			memcpy(a_elstPos_dur, &duration, sizeof(duration) );*/

			a_tkhdPos_dur = a_tkhdPos + 28;
			memcpy(a_tkhdPos_dur, &duration, sizeof(duration) );

			a_trak_size = SWAP32(a_trak_size);
			memcpy(a_trakPos, &a_trak_size, sizeof(a_trak_size) );
		}

		mvhdPos_dur = mvhdPos + 24;
		memcpy(mvhdPos_dur, &duration, sizeof(duration) );

		moov_size = SWAP32(moov_size);
		memcpy(moovPos, &moov_size, sizeof(moov_size) );

// 串接檔案 ==================================================================================
		if(check_freesize2(freesize, (header_ptr - &mp4_header_buf[0]) ) != 0) return -2;
		ret = fwrite(&mp4_header_buf[0], (header_ptr - &mp4_header_buf[0]), 1, mp4_fp);
		if(ret == 0) return -4;

		if(check_freesize2(freesize, (v_header_ptr - &v_header_buf[0]) ) != 0) return -2;
		ret = fwrite(  &v_header_buf[0], (v_header_ptr - &v_header_buf[0]), 1, mp4_fp);
		if(ret == 0) return -4;

		if(check_freesize2(freesize, (v_stts_e_ptr - &v_stts_e_buf[0]) ) != 0) return -2;
		ret = fwrite(  &v_stts_e_buf[0], (v_stts_e_ptr - &v_stts_e_buf[0]), 1, mp4_fp);
		if(ret == 0) return -4;

		if(check_freesize2(freesize, (v_stss_e_ptr - &v_stss_e_buf[0]) ) != 0) return -2;
		ret = fwrite(  &v_stss_e_buf[0], (v_stss_e_ptr - &v_stss_e_buf[0]), 1, mp4_fp);
		if(ret == 0) return -4;

		if(check_freesize2(freesize, (v_stsc_e_ptr - &v_stsc_e_buf[0]) ) != 0) return -2;
		ret = fwrite(  &v_stsc_e_buf[0], (v_stsc_e_ptr - &v_stsc_e_buf[0]), 1, mp4_fp);
		if(ret == 0) return -4;

		if(check_freesize2(freesize, (v_stsz_e_ptr - &v_stsz_e_buf[0]) ) != 0) return -2;
		ret = fwrite(  &v_stsz_e_buf[0], (v_stsz_e_ptr - &v_stsz_e_buf[0]), 1, mp4_fp);
		if(ret == 0) return -4;

		if(check_freesize2(freesize, (v_stco_e_ptr - &v_stco_e_buf[0]) ) != 0) return -2;
		ret = fwrite(  &v_stco_e_buf[0], (v_stco_e_ptr - &v_stco_e_buf[0]), 1, mp4_fp);
		if(ret == 0) return -4;

//		if( (play_mode == 0 || play_mode == 2) && width != 6144) {
			if(check_freesize2(freesize, sizeof(Metadata_buf) ) != 0) return -2;
			fwrite(&Metadata_buf[0], sizeof(Metadata_buf), 1, mp4_fp);
//		}

		if(abuf != NULL) {
			if(check_freesize2(freesize, (a_header_ptr - &a_header_buf[0]) ) != 0) return -1;
			ret = fwrite(&a_header_buf[0], (a_header_ptr - &a_header_buf[0]), 1, mp4_fp);
			if(ret == 0) return -4;

			if(check_freesize2(freesize, (a_stts_e_ptr - &a_stts_e_buf[0]) ) != 0) return -1;
			ret = fwrite(&a_stts_e_buf[0], (a_stts_e_ptr - &a_stts_e_buf[0]), 1, mp4_fp);
			if(ret == 0) return -4;

			if(check_freesize2(freesize, (a_stsc_e_ptr - &a_stsc_e_buf[0]) ) != 0) return -1;
			ret = fwrite(&a_stsc_e_buf[0], (a_stsc_e_ptr - &a_stsc_e_buf[0]), 1, mp4_fp);
			if(ret == 0) return -4;

			if(check_freesize2(freesize, (a_stsz_e_ptr - &a_stsz_e_buf[0]) ) != 0) return -1;
			ret = fwrite(&a_stsz_e_buf[0], (a_stsz_e_ptr - &a_stsz_e_buf[0]), 1, mp4_fp);
			if(ret == 0) return -4;

			if(check_freesize2(freesize, (a_stco_e_ptr - &a_stco_e_buf[0]) ) != 0) return -1;
			ret = fwrite(&a_stco_e_buf[0], (a_stco_e_ptr - &a_stco_e_buf[0]), 1, mp4_fp);
			if(ret == 0) return -4;
		}

		set_udta_box(&udta_box_s, play_mode);
		if(check_freesize2(freesize, sizeof(udta_box_s) ) != 0) return -2;
		ret = fwrite(&udta_box_s, sizeof(udta_box_s), 1, mp4_fp);
		if(ret == 0) return -4;

		fsetpos(mp4_fp, &mdatPos);
		mdat_size += sizeof(mdat_header_box_s);
		mdat_size = SWAP32(mdat_size);
		if(check_freesize2(freesize, sizeof(mdat_size) ) != 0) return -2;
		ret = fwrite(&mdat_size, sizeof(mdat_size), 1, mp4_fp);
		if(ret == 0) return -4;

// close =====================================================================================
		header_ptr   = NULL;
		v_header_ptr = NULL;
		a_header_ptr = NULL;

		v_stts_e_ptr = NULL;
		v_stss_e_ptr = NULL;
		v_stsc_e_ptr = NULL;
		v_stsz_e_ptr = NULL;
		v_stco_e_ptr = NULL;
		v_stts_e_ptr_lst = NULL;

		a_stts_e_ptr = NULL;
		a_stsc_e_ptr = NULL;
		a_stsz_e_ptr = NULL;
		a_stco_e_ptr = NULL;

		fflush(mp4_fp);
#ifdef ANDROID_CODE
		mp4_fd = fileno(mp4_fp);
		fsync(mp4_fd);
#endif

		fclose(mp4_fp);
#ifdef ANDROID_CODE
		close(mp4_fd);
		db_debug("save_mp4_proc() e\n");
#endif
		break;
	}

	return 0;
}
