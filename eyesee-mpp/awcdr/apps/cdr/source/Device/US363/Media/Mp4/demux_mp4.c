/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Media/Mp4/demux_mp4.h"

#include <stdio.h>
#include <stdint.h>
#include <malloc.h>

#include "Device/US363/Media/Mp4/mp4.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::DemuxMp4"

char* moov_buf = NULL;

demux_info_struct demux_info;

mp4_header_box_struct demux_mdat;
mp4_header_box_struct demux_moov;
mvhd_box_struct           demux_mvhd;

demux_trak_info_struct trak_box[DEMUX_TRAK_MAX] = { 0 };


int find_mdat(mp4_header_box_struct *mdat, char *buf, uint32_t size, int *offset) {
    int i, flag = 0;
    uint32_t *tmp_t = NULL;

    for (i = 0; i < size; i++) {
        tmp_t = buf + i;
        if (*tmp_t == MDAT_T) {
            set_mdat_box(mdat, *(tmp_t - 1));
            db_debug("M4TS: mdat size=%d\n", demux_mdat.size);
            *offset = i;
            flag = 1;
            break;
        }
    }
    return flag;
}

int find_mdhd_trak(mvhd_box_struct *mvhd, demux_trak_info_struct *trak, char *buf, uint32_t size) {
    int trak_cnt = 0;
    char* ptr = NULL;
    uint32_t* tmp_t = NULL;
    demux_trak_info_struct *trak_ptr = NULL;

    ptr = buf + 8;     // +8: 跳過 "moov" + mvdh_size, 減少搜尋次數
    while (ptr < (buf + size)) {
        tmp_t = ptr;
        if (*tmp_t == MVHD_T) {
            memcpy(mvhd, (tmp_t - 1), sizeof(mvhd_box_struct));
            mvhd->mvhd_header.size = SWAP32(mvhd->mvhd_header.size);
            db_debug("M4TS: mvhd size=%d\n", mvhd->mvhd_header.size);
            ptr = tmp_t - 1;
            ptr += (mvhd->mvhd_header.size + 4);           // +4: 跳過下一個 box size, 減少搜尋次數
        }
        else if (*tmp_t == TRAK_T && trak_cnt < DEMUX_TRAK_MAX) {
            trak_ptr = trak + trak_cnt;

            trak_ptr->ptr = tmp_t - 1;
            trak_ptr->size = SWAP32(*(tmp_t - 1));
            db_debug("M4TS: trak size=%d\n", trak_ptr->size);
            ptr = trak_ptr->ptr + trak_ptr->size + 4;       // +4: 跳過下一個 box size, 減少搜尋次數
            trak_ptr->flag = 1;
            trak_cnt++;
            if (trak_cnt >= DEMUX_TRAK_MAX) break;
        }
        else
            ptr++;
    }
    return trak_cnt;
}

int set_trak_info(demux_trak_info_struct* trak) {
    int i, j, tmp_size = 0;
    char* ptr = NULL;
    uint32_t* tmp_t = NULL;
    demux_trak_info_struct* trak_ptr = NULL;

    for (i = 0; i < DEMUX_TRAK_MAX; i++) {
        trak_ptr = trak + i;
        if (trak_ptr->flag == 0) continue;

        db_debug("M4TS: ------------------------ trak %d ------------------------\n", i);
        ptr = trak_ptr->ptr;
        while (ptr < (trak_ptr->ptr + trak_ptr->size)) {
            tmp_t = ptr;
            if (*tmp_t == MDHD_T) {
                trak_ptr->mdhd_info.ptr = tmp_t - 1;
                memcpy(&trak_ptr->mdhd_info.mdhd, (tmp_t - 1), sizeof(trak_ptr->mdhd_info.mdhd));
                trak_ptr->mdhd_info.mdhd.mdhd_header.size = SWAP32(trak_ptr->mdhd_info.mdhd.mdhd_header.size);
                trak_ptr->mdhd_info.mdhd.time_sczle = SWAP32(trak_ptr->mdhd_info.mdhd.time_sczle);
                trak_ptr->mdhd_info.mdhd.duration = SWAP32(trak_ptr->mdhd_info.mdhd.duration);
                db_debug("M4TS: mdhd size=%d sczle=%d duration=%d\n", trak_ptr->mdhd_info.mdhd.mdhd_header.size, trak_ptr->mdhd_info.mdhd.time_sczle, trak_ptr->mdhd_info.mdhd.duration);
                ptr = tmp_t - 1;
                ptr += (trak_ptr->mdhd_info.mdhd.mdhd_header.size + 4);           // +4: 跳過下一個 box size, 減少搜尋次數
            }
            else if (*tmp_t == HDLR_T) {
                trak_ptr->hdlr_info.ptr = tmp_t - 1;
                memcpy(&trak_ptr->hdlr_info.hdlr, (tmp_t - 1), sizeof(trak_ptr->hdlr_info.hdlr));
                trak_ptr->hdlr_info.hdlr.hdlr_header.size = SWAP32(trak_ptr->hdlr_info.hdlr.hdlr_header.size);
                db_debug("M4TS: hdlr size=%d\n", trak_ptr->hdlr_info.hdlr.hdlr_header.size);
                ptr = tmp_t - 1;
                ptr += (trak_ptr->hdlr_info.hdlr.hdlr_header.size + 4);           // +4: 跳過下一個 box size, 減少搜尋次數
            }
            else if (*tmp_t == STSD_T) {
                trak_ptr->stsd_info.ptr = tmp_t - 1;
                memcpy(&trak_ptr->stsd_info.stsd, (tmp_t - 1), sizeof(trak_ptr->stsd_info.stsd));
                trak_ptr->stsd_info.stsd.stsd_header.size = SWAP32(trak_ptr->stsd_info.stsd.stsd_header.size);
                db_debug("M4TS: stsd size=%d\n", trak_ptr->stsd_info.stsd.stsd_header.size);
                ptr++;
            }
            else if (*tmp_t == AVC1_T) {
                trak_ptr->video_entry_info.ptr = tmp_t - 1;
                memcpy(&trak_ptr->video_entry_info.video_entry, (tmp_t - 1), sizeof(trak_ptr->video_entry_info.video_entry));
                trak_ptr->video_entry_info.video_entry.size = SWAP32(trak_ptr->video_entry_info.video_entry.size);
                trak->video_entry_info.video_entry.width = SWAP16(trak->video_entry_info.video_entry.width);
                trak->video_entry_info.video_entry.height = SWAP16(trak->video_entry_info.video_entry.height);
                demux_info.width  = trak->video_entry_info.video_entry.width;
                demux_info.height = trak->video_entry_info.video_entry.height;
                db_debug("M4TS: avc1 size=%d width=%d height=%d\n", trak_ptr->video_entry_info.video_entry.size, demux_info.width, demux_info.height);

                ptr = tmp_t - 1;
                ptr += sizeof(visual_sample_entry_struct);
                ptr += sizeof(avcC_box_struct);
                // sps
                memcpy(&demux_info.sps_len, (ptr+1), sizeof(uint16_t));
                demux_info.sps_len = SWAP16(demux_info.sps_len);
                ptr += sizeof(uint16_t);
                ptr++;
                memcpy(&demux_info.sps_buf[0], ptr, demux_info.sps_len);
                ptr += demux_info.sps_len;
                //ptr++;
                db_debug("M4TS: sps: %d\n", demux_info.sps_len);

                // pps
                memcpy(&demux_info.pps_len, (ptr + 1), sizeof(uint16_t));
                demux_info.pps_len = SWAP16(demux_info.pps_len);
                ptr += sizeof(uint16_t);
                ptr++;
                memcpy(&demux_info.pps_buf[0], ptr, demux_info.pps_len);
                ptr += demux_info.pps_len;
                //ptr++;
                db_debug("M4TS: pps: %d\n", demux_info.pps_len);
            }
            else if (*tmp_t == SOWT_T) {
                trak_ptr->audio_entry_info.ptr = tmp_t - 1;
                memcpy(&trak_ptr->audio_entry_info.audio_entry, (tmp_t - 1), sizeof(trak_ptr->audio_entry_info.audio_entry));
                trak_ptr->audio_entry_info.audio_entry.size = SWAP32(trak_ptr->audio_entry_info.audio_entry.size);
                trak_ptr->audio_entry_info.audio_entry.samplerate = SWAP16(trak_ptr->audio_entry_info.audio_entry.samplerate);
                trak_ptr->audio_entry_info.audio_entry.channelcount = SWAP16(trak_ptr->audio_entry_info.audio_entry.channelcount);
                trak_ptr->audio_entry_info.audio_entry.samplesize = SWAP16(trak_ptr->audio_entry_info.audio_entry.samplesize);
                demux_info.sample_rate = trak_ptr->audio_entry_info.audio_entry.samplerate;
                demux_info.channel       = trak_ptr->audio_entry_info.audio_entry.channelcount;
                demux_info.bits             = trak_ptr->audio_entry_info.audio_entry.samplesize;
                db_debug("M4TS: sowt size=%d rate=%d ch=%d bit=%d\n", trak_ptr->audio_entry_info.audio_entry.size, demux_info.sample_rate, demux_info.channel, demux_info.bits);
                ptr = tmp_t - 1;
                ptr += (trak_ptr->audio_entry_info.audio_entry.size + 4);           // +4: 跳過下一個 box size, 減少搜尋次數
            }
            else if (*tmp_t == MP4A_T) {
                trak_ptr->audio_entry_info.ptr = tmp_t - 1;
                memcpy(&trak_ptr->audio_entry_info.audio_entry, (tmp_t - 1), sizeof(trak_ptr->audio_entry_info.audio_entry));
                trak_ptr->audio_entry_info.audio_entry.size = SWAP32(trak_ptr->audio_entry_info.audio_entry.size);
                trak_ptr->audio_entry_info.audio_entry.samplerate = SWAP16(trak_ptr->audio_entry_info.audio_entry.samplerate);
                trak_ptr->audio_entry_info.audio_entry.channelcount = SWAP16(trak_ptr->audio_entry_info.audio_entry.channelcount);
                trak_ptr->audio_entry_info.audio_entry.samplesize = SWAP16(trak_ptr->audio_entry_info.audio_entry.samplesize);
                demux_info.sample_rate = trak_ptr->audio_entry_info.audio_entry.samplerate;
                demux_info.channel = trak_ptr->audio_entry_info.audio_entry.channelcount;
                demux_info.bits = trak_ptr->audio_entry_info.audio_entry.samplesize;
                db_debug("M4TS: mp4a size=%d rate=%d ch=%d bit=%d\n", trak_ptr->audio_entry_info.audio_entry.size, demux_info.sample_rate, demux_info.channel, demux_info.bits);
                ptr = tmp_t - 1;
                ptr += (trak_ptr->audio_entry_info.audio_entry.size + 4);           // +4: 跳過下一個 box size, 減少搜尋次數
            }
            else if (*tmp_t == STTS_T) {
                trak_ptr->stts_info.ptr = tmp_t - 1;
                trak_ptr->stts_info.sample_prt = trak_ptr->stts_info.ptr + sizeof(trak_ptr->stts_info.stts);
                memcpy(&trak_ptr->stts_info.stts, (tmp_t - 1), sizeof(trak_ptr->stts_info.stts));
                trak_ptr->stts_info.stts.stts_header.size = SWAP32(trak_ptr->stts_info.stts.stts_header.size);
                trak_ptr->stts_info.stts.entry_count = SWAP32(trak_ptr->stts_info.stts.entry_count);
                db_debug("M4TS: stts size=%d count=%d\n", trak_ptr->stts_info.stts.stts_header.size, trak_ptr->stts_info.stts.entry_count);
                ptr = tmp_t - 1;
                ptr += (trak_ptr->stts_info.stts.stts_header.size + 4);           // +4: 跳過下一個 box size, 減少搜尋次數
            }
            else if (*tmp_t == STSS_T) {
                trak_ptr->stss_info.ptr = tmp_t - 1;
                trak_ptr->stss_info.sample_prt = trak_ptr->stss_info.ptr + sizeof(trak_ptr->stss_info.stss);
                memcpy(&trak_ptr->stss_info.stss, (tmp_t - 1), sizeof(trak_ptr->stss_info.stss));
                trak_ptr->stss_info.stss.stss_header.size = SWAP32(trak_ptr->stss_info.stss.stss_header.size);
                trak_ptr->stss_info.stss.entry_count = SWAP32(trak_ptr->stss_info.stss.entry_count);
                db_debug("M4TS: stss size=%d count=%d\n", trak_ptr->stss_info.stss.stss_header.size, trak_ptr->stss_info.stss.entry_count);
                ptr = tmp_t - 1;
                ptr += (trak_ptr->stss_info.stss.stss_header.size + 4);           // +4: 跳過下一個 box size, 減少搜尋次數
            }
            else if (*tmp_t == STSC_T) {
                trak_ptr->stsc_info.ptr = tmp_t - 1;
                trak_ptr->stsc_info.sample_prt = trak_ptr->stsc_info.ptr + sizeof(trak_ptr->stsc_info.stsc);
                memcpy(&trak_ptr->stsc_info.stsc, (tmp_t - 1), sizeof(trak_ptr->stsc_info.stsc));
                trak_ptr->stsc_info.stsc.stsc_header.size = SWAP32(trak_ptr->stsc_info.stsc.stsc_header.size);
                trak_ptr->stsc_info.stsc.entry_count = SWAP32(trak_ptr->stsc_info.stsc.entry_count);
                db_debug("M4TS: stsc size=%d count=%d\n", trak_ptr->stsc_info.stsc.stsc_header.size, trak_ptr->stsc_info.stsc.entry_count);
                ptr = tmp_t - 1;
                ptr += (trak_ptr->stsc_info.stsc.stsc_header.size + 4);           // +4: 跳過下一個 box size, 減少搜尋次數
            }
            else if (*tmp_t == STSZ_T) {
                trak_ptr->stsz_info.ptr = tmp_t - 1;
                trak_ptr->stsz_info.sample_prt = trak_ptr->stsz_info.ptr + sizeof(trak_ptr->stsz_info.stsz);
                memcpy(&trak_ptr->stsz_info.stsz, (tmp_t - 1), sizeof(trak_ptr->stsz_info.stsz));
                trak_ptr->stsz_info.stsz.stsz_header.size = SWAP32(trak_ptr->stsz_info.stsz.stsz_header.size);
                trak_ptr->stsz_info.stsz.entry_count = SWAP32(trak_ptr->stsz_info.stsz.entry_count);
                db_debug("M4TS: stsz size=%d count=%d\n", trak_ptr->stsz_info.stsz.stsz_header.size, trak_ptr->stsz_info.stsz.entry_count);
                ptr = tmp_t - 1;
                ptr += (trak_ptr->stsz_info.stsz.stsz_header.size + 4);           // +4: 跳過下一個 box size, 減少搜尋次數
            }
            else if (*tmp_t == STCO_T) {
                trak_ptr->stco_info.ptr = tmp_t - 1;
                trak_ptr->stco_info.sample_prt = trak_ptr->stco_info.ptr + sizeof(trak_ptr->stco_info.stco);
                memcpy(&trak_ptr->stco_info.stco, (tmp_t - 1), sizeof(trak_ptr->stco_info.stco));
                trak_ptr->stco_info.stco.stco_header.size = SWAP32(trak_ptr->stco_info.stco.stco_header.size);
                trak_ptr->stco_info.stco.entry_count = SWAP32(trak_ptr->stco_info.stco.entry_count);
                db_debug("M4TS: stco size=%d count=%d\n", trak_ptr->stco_info.stco.stco_header.size, trak_ptr->stco_info.stco.entry_count);
                ptr = tmp_t - 1;
                ptr += (trak_ptr->stco_info.stco.stco_header.size + 4);           // +4: 跳過下一個 box size, 減少搜尋次數
            }
            else if (*tmp_t == VMHD_T || *tmp_t == SMHD_T || *tmp_t == DINF_T) {        // 沒用到, 直接跳過加速搜尋
                trak_ptr->hdlr_info.ptr = tmp_t - 1;
                memcpy(&tmp_size, (tmp_t - 1), sizeof(tmp_size));
                tmp_size = SWAP32(tmp_size);
                ptr = tmp_t - 1;
                ptr += (tmp_size + 4);           // +4: 跳過下一個 box size, 減少搜尋次數
            }
            else
                ptr++;
        }
    }
    return 0;
}

int demux_mp4(char* src_path) {
    FILE* fp = NULL;
    int offset;
    int isMdat = 0;
    char top_buf[64];
    uint32_t moov_size = 0;

    fp = fopen(src_path, "rb");
    if (fp != NULL) {
        // 1.get mdat size
        fread(&top_buf[0], sizeof(top_buf), 1, fp);
        offset = 0;
        isMdat = find_mdat(&demux_mdat, &top_buf[0], sizeof(top_buf), &offset);

        // 2.get header
        if (isMdat == 1) {
            // moov
            fseek(fp, demux_mdat.size + offset - 4, SEEK_SET);
            fread(&moov_size, sizeof(uint32_t), 1, fp);
            set_moov_box(&demux_moov, moov_size);
            db_debug("M4TS: moov size=%d\n", demux_moov.size);
            moov_buf = malloc(demux_moov.size);
            if (moov_buf != NULL) {
                fread(&moov_buf[0], demux_moov.size, 1, fp);
                // mvhd, trak
                find_mdhd_trak(&demux_mvhd, &trak_box[0], &moov_buf[0], demux_moov.size);
                set_trak_info(&trak_box[0]);
            }
            else {
                db_error("M4TS: malloc moov buf error\n");
                goto error;
            }

            // 3.get data

        }
        else {
            db_error("M4TS: not found mdat\n");
        }
        
    }

    return 0;
error:
    if (fp != NULL)
        fclose(fp);
    return -1;
}

void demux_destroy(void) {
    if (moov_buf != NULL)
        free(moov_buf);
}
