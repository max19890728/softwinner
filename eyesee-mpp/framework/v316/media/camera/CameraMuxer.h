/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/
#ifndef __IPCLINUX_CAMERA_MUXER_H__
#define __IPCLINUX_CAMERA_MUXER_H__

#include <vector>
#include <map>

#include <vencoder.h>
#include <tsemaphore.h>
#include <mm_comm_video.h>
#include <mm_comm_venc.h>
#include <mm_comm_aenc.h>
#include <mm_comm_sys.h>
#include <mm_comm_mux.h>

#include <Errors.h>


#define MAX_FILE_PATH_LEN  (128)

namespace EyeseeLinux {
	
typedef struct output_sink_info_s
{
    int mMuxerId;
    MEDIA_FILE_FORMAT_E mOutputFormat;
    int mOutputFd;
    int mFallocateLen;
    BOOL mCallbackOutFlag;
}OUTSINKINFO_S, *PTR_OUTSINKINFO_S;

typedef struct mux_chn_info_s
{
    OUTSINKINFO_S mSinkInfo;
    MUX_CHN_ATTR_S mMuxChnAttr;
    MUX_CHN mMuxChn;
    //struct list_head mList;
}MUX_CHN_INFO_S, *PTR_MUX_CHN_INFO_S;


/*typedef enum H264_PROFILE_E
{
   H264_PROFILE_BASE = 0,
   H264_PROFILE_MAIN,
   H264_PROFILE_HIGH,
}H264_PROFILE_E;

typedef enum H265_PROFILE_E
{
   H265_PROFILE_MAIN = 0,
   H265_PROFILE_MAIN10,
   H265_PROFILE_STI11,
}H265_PROFILE_E;*/

/*typedef struct IN_FRAME_NODE_S
{
    VIDEO_FRAME_INFO_S  mFrame;
    struct list_head mList;
}IN_FRAME_NODE_S;*/

typedef struct INPUT_BUF_Q
{
    int mFrameNum;

    struct list_head mIdleList;
    struct list_head mReadyList;
    struct list_head mUseList;
    pthread_mutex_t mIdleListLock;
    pthread_mutex_t mReadyListLock;
    pthread_mutex_t mUseListLock;
}INPUT_BUF_Q;

typedef enum recordstate
{
    REC_NOT_PREPARED = 0,
    REC_PREPARED,
    REC_RECORDING,
    REC_STOP,
    REC_ERROR,
}RECSTATE_E;

typedef struct CameraMuxerConfig
{
	//video
    int mVideoSize;
    int mVideoWidth;
    int mVideoHeight;

    int mField;
    PAYLOAD_TYPE_E mVideoEncoderFmt;
    int mVideoFrameRate;
    int mVideoBitRate;
    int mEncUseProfile;
    int mRcMode;
	
	//audio
	int mChannels;
    int mBitsPerSample; 
    int mSamplesPerFrame; 				//sample_cnt_per_frame
    int mSampleRate;    
    PAYLOAD_TYPE_E mAudioEncodeType;  	//AUDIO_ENCODER_AAC_TYPE
	
	//muxer
	char dstVideoFile[MAX_FILE_PATH_LEN];
	MEDIA_FILE_FORMAT_E mMediaFileFmt;
	int mMaxFileDuration;

}CameraMuxerConfig;


typedef struct MuxContext
{
    CameraMuxerConfig mConfig;

    MPP_SYS_CONF_S mSysConf;

    RECSTATE_E mCurRecState;

    int mMuxerIdCounter;

    MUX_GRP_ATTR_S mMuxGrpAttr;
    MUX_GRP mMuxGrp;

    VENC_CHN_ATTR_S mVencChnAttr;
    VENC_CHN mVeChn;
	
	AENC_CHN_ATTR_S mAencChnAttr;
    AENC_CHN mAeChn;

    pthread_mutex_t mChnListLock;
    //struct list_head mMuxChnList;
	std::vector<MUX_CHN_INFO_S> mMuxChnList;

    char *tmpBuf;

    INPUT_BUF_Q mInBuf_Q;
    pthread_t mReadThdId;

    BOOL mOverFlag;

}MuxContext;


class CameraMuxer
{
public:
    CameraMuxer();
    ~CameraMuxer();
	
	static CameraMuxer& instance() {
		static CameraMuxer instance_;
		return instance_;
	}

	int getVideoSize(int width, int height);
	ERRORTYPE initConfig(CameraMuxerConfig *pConfig);
	ERRORTYPE start();
	ERRORTYPE stop();
    status_t initialize(CameraMuxerConfig *pConfig, VENC_CHN vChn, AENC_CHN aChn);
    status_t destroy();

    MUX_GRP getMuxerGrp(){ return stMuxContext.mMuxGrp;}

    static ERRORTYPE MuxCallback(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData);
	
private:
	int getFileNameByCurTime(char *pNameBuf);
	ERRORTYPE setConfig(CameraMuxerConfig *pConfig);
	ERRORTYPE configMuxGrpAttr(MuxContext *pContext);
	ERRORTYPE createMuxGrp(MuxContext *pContext);
	int addOutputFormatAndOutputSink_1(MuxContext *pContext, OUTSINKINFO_S *pSinkInfo);
	int addOutputFormatAndOutputSink(MuxContext *pContext, char* path, MEDIA_FILE_FORMAT_E mediaFileFmt);
	int setOutputFileSync_1(MuxContext *pContext, int fd, int64_t fallocateLength, int muxerId);
	int setOutputFileSync(MuxContext *pContext, char* path, int64_t fallocateLength, int muxerId);
	int configVencChnAttr(MuxContext *pContext);
	ERRORTYPE createVencChn(MuxContext *pContext, VENC_CHN vChn);
	int configAencChnAttr(MuxContext *pContext);
	ERRORTYPE createAencChn(MuxContext *pContext, AENC_CHN aChn);
	ERRORTYPE prepare(MuxContext *pContext, VENC_CHN vChn, AENC_CHN aChn);
	
	MuxContext stMuxContext;
	//static int file_cnt;
	
};

}; /* namespace EyeseeLinux */

#endif /* __IPCLINUX_CAMERA_MUXER_H__ */

