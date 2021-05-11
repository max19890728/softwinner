/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/
#ifndef __IPCLINUX_CAMERA_H264_ENCODER_H__
#define __IPCLINUX_CAMERA_H264_ENCODER_H__

#include <vencoder.h>
#include <tsemaphore.h>
#include <mm_comm_video.h>
#include <mm_comm_venc.h>
#include <mm_comm_sys.h>

#include <Errors.h>

namespace EyeseeLinux {

typedef enum H264_PROFILE_E
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
}H265_PROFILE_E;

typedef struct H264_SPS_PPS_S
{
    char sps[32];
	int sps_len;
	
	char pps[32];
	int pps_len;
	
}H264_SPS_PPS_S;

typedef struct CameraH264EncConfig
{
    //char outputFile[MAX_FILE_PATH_LEN];
    //FILE * fd_out;

    int srcWidth;
    int srcHeight;
    int srcSize;
    int srcPixFmt;

    int dstWidth;
    int dstHeight;
    int dstSize;
    int dstPixFmt;

    //int mEncodeFrameNum;
    PAYLOAD_TYPE_E mVideoEncoderFmt;
    int mEncUseProfile;
    int mField;
    int mVideoMaxKeyItl;
    int mVideoBitRate;
    int mVideoFrameRate;
    int maxKeyFrame;
    int mTimeLapseEnable;
    int mTimeBetweenFrameCapture;

    int mRcMode;
    //for cbr/vbr:qp0=minQp, qp1=maxQp; for fixqp:qp0=IQp, qp1=PQp; for mjpeg cbr:qp0=init_Qfactor
    int mQp0;
    int mQp1;
    ROTATE_E rotate;
	
}CameraH264EncConfig;


typedef struct IN_FRAME_NODE_S
{
    VIDEO_FRAME_INFO_S  mFrame;
    struct list_head mList;
}IN_FRAME_NODE_S;

/*typedef struct INPUT_BUF_Q
{
    int mFrameNum;

    struct list_head mIdleList;
    struct list_head mReadyList;
    struct list_head mUseList;
    pthread_mutex_t mIdleListLock;
    pthread_mutex_t mReadyListLock;
    pthread_mutex_t mUseListLock;
}INPUT_BUF_Q;*/

typedef struct VEncContext
{
	CameraH264EncConfig mConfig;
	
    MPP_SYS_CONF_S mSysConf;
    VENC_CHN_ATTR_S mVencChnAttr;
    VENC_CHN mVeChn;
	
//    char *tmpBuf;

//    INPUT_BUF_Q mInBuf_Q;
	
}VEncContext;
	

class CameraH264Encoder
{
public:
    CameraH264Encoder();
    ~CameraH264Encoder();
	
	static CameraH264Encoder& instance() {
		static CameraH264Encoder instance_;
		return instance_;
	}

	ERRORTYPE initConfig(CameraH264EncConfig *pConfig);
    status_t initialize(CameraH264EncConfig *pConfig);
    status_t destroy();
    status_t encode(VIDEO_FRAME_INFO_S *pFrameInfo);
	void getSpsPpsHead(VencHeaderData *spspps);
	int getH264SpsPps(VencHeaderData src, H264_SPS_PPS_S *dst);
    status_t getFrame(VENC_STREAM_S *pStream);
    void returnFrame(VENC_STREAM_S *pStream);
	int getVideoSize(int width, int height);
	int getDataSize(VENC_STREAM_S *pStream);
	int getData(VENC_STREAM_S *pStream, unsigned char *pData);

    VENC_CHN getH264VenChn(){ return stContext.mVeChn;}

    static ERRORTYPE VEncCallback(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData);
	
private:
	ERRORTYPE setConfig(CameraH264EncConfig *pConfig);
//	int vencBufMgrCreate(int frmNum, int videoSize, INPUT_BUF_Q *pBufList, BOOL isAwAfbc);
//	int vencBufMgrDestroy(INPUT_BUF_Q *pBufList);
	ERRORTYPE configVencChnAttr(VEncContext *pContext);
	ERRORTYPE createVencChn(VEncContext *pContext);

	VEncContext stContext;
	
};

}; /* namespace EyeseeLinux */

#endif /* __IPCLINUX_CAMERA_H264_ENCODER_H__ */

