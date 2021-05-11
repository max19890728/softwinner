/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/
#ifndef __IPCLINUX_CAMERA_JPEG_DECODER_H__
#define __IPCLINUX_CAMERA_JPEG_DECODER_H__

#include <vencoder.h>
#include <tsemaphore.h>
#include <mm_comm_video.h>
#include <mm_comm_vdec.h>
#include <mm_comm_sys.h>

#include <Errors.h>

namespace EyeseeLinux {

typedef struct CameraJpegDecConfig
{
    PIXEL_FORMAT_E mPixelFormat;
    unsigned int mPicWidth;
    unsigned int mPicHeight;
    unsigned int nVbvBufferSize;
} CameraJpegDecConfig;

typedef struct VDecContext
{
    MPP_SYS_CONF_S mSysConf;
    VDEC_CHN mVDecChn;
    VDEC_CHN_ATTR_S mVDecAttr;
}VDecContext;

class CameraJpegDecoder
{
public:
    CameraJpegDecoder();
    ~CameraJpegDecoder();
	
	static CameraJpegDecoder& instance() {
		static CameraJpegDecoder instance_;
		return instance_;
	}

    status_t initialize(CameraJpegDecConfig * pConfig);
    status_t destroy();
	status_t loadSrcFile(VDEC_STREAM_S *pStreamInfo, char *srcPath);
	void freeSrcFileBuf(VDEC_STREAM_S *pStreamInfo);
    status_t decode(VDEC_STREAM_S *pStreamInfo);
    status_t getFrame(VIDEO_FRAME_INFO_S *pFrameInfo);
    void returnFrame(VIDEO_FRAME_INFO_S *pFrameInfo);
    void getDataSize(VIDEO_FRAME_INFO_S *pFrameInfo, int *size_array);

    VENC_CHN getJpegVdeChn(){ return stContext.mVDecChn;}
	
    static ERRORTYPE VDecCallback(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData);
	
private:	
	VDecContext stContext;
	
};

}; /* namespace EyeseeLinux */

#endif /* __IPCLINUX_CAMERA_JPEG_DECODER_H__ */

