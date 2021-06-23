/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#define LOG_NDEBUG 0
#define LOG_TAG "CameraJpegDecoder"
#include <utils/plat_log.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <malloc.h>
#include <cstdlib>
#include <cstring>
#include <mpi_vdec.h>
#include <mpi_sys.h>

#include "CameraJpegDecoder.h"


using namespace std;
namespace EyeseeLinux {

CameraJpegDecoder::CameraJpegDecoder()
{
    memset(&stContext, 0, sizeof(stContext));
}

CameraJpegDecoder::~CameraJpegDecoder()
{
	//
}

ERRORTYPE CameraJpegDecoder::VDecCallback(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData)
{
    ERRORTYPE ret = SUCCESS;
    CameraJpegDecoder *pThiz = (CameraJpegDecoder*)cookie;
    if(MOD_ID_VDEC == pChn->mModId)
    {
        if(pChn->mChnId != pThiz->stContext.mVDecChn)
        {
            aloge("fatal error! VDec chnId[%d]!=[%d]", pChn->mChnId, pThiz->stContext.mVDecChn);
        }
        switch(event)
        {
            case MPP_EVENT_NOTIFY_EOF:
            {
                alogd("VDec channel notify APP that decode complete!");
                break;
            }
            default:
            {
                aloge("fatal error! unknown event[0x%x] from channel[0x%x][0x%x][0x%x]!", event, pChn->mModId, pChn->mDevId, pChn->mChnId);
                ret = ERR_VDEC_ILLEGAL_PARAM;
                break;
            }
        }
    }
    else
    {
        aloge("fatal error! why modId[0x%x]?!", pChn->mModId);
        ret = FAILURE;
    }
    return ret;
}

status_t CameraJpegDecoder::initialize(CameraJpegDecConfig * pConfig)
{
    //init mpp system
    memset(&stContext.mSysConf, 0, sizeof(MPP_SYS_CONF_S));
    stContext.mSysConf.nAlignWidth = 32;
    AW_MPI_SYS_SetConf(&stContext.mSysConf);
    AW_MPI_SYS_Init();
	
	//config vdec chn attr.
    memset(&stContext.mVDecAttr, 0, sizeof(stContext.mVDecAttr));
    stContext.mVDecAttr.mType      = PT_JPEG;
    stContext.mVDecAttr.mOutputPixelFormat = pConfig->mPixelFormat;	//MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    stContext.mVDecAttr.mBufSize   = pConfig->nVbvBufferSize;
    stContext.mVDecAttr.mPicWidth  = pConfig->mPicWidth;
    stContext.mVDecAttr.mPicHeight = pConfig->mPicHeight;
	
	//create vdec channel.
    ERRORTYPE ret;
    BOOL bSuccessFlag = FALSE;
    stContext.mVDecChn = 0;
    while (stContext.mVDecChn < VDEC_MAX_CHN_NUM)
    {
        ret = AW_MPI_VDEC_CreateChn(stContext.mVDecChn, &stContext.mVDecAttr);
        if(SUCCESS == ret)
        {
            bSuccessFlag = TRUE;
            alogd("create VDec channel[%d] success!", stContext.mVDecChn);
            break;
        }
        else if (ERR_VDEC_EXIST == ret)
        {
            alogd("VDec channel[%d] exist, find next!", stContext.mVDecChn);
            stContext.mVDecChn++;
        }
        else
        {
            aloge("create VDec channel[%d] fail! ret[0x%x]!", stContext.mVDecChn, ret);
            break;
        }
    }
    if (FALSE == bSuccessFlag)
    {
        stContext.mVDecChn = MM_INVALID_CHN;
        aloge("fatal error! create VDec channel fail!");
		return UNKNOWN_ERROR;
    }

    MPPCallbackInfo cbInfo;
    cbInfo.cookie = (void*)this;
    cbInfo.callback = (MPPCallbackFuncType)&VDecCallback;
    AW_MPI_VDEC_RegisterCallback(stContext.mVDecChn, &cbInfo);
    AW_MPI_VDEC_StartRecvStream(stContext.mVDecChn);
	return NO_ERROR;
}

status_t CameraJpegDecoder::destroy()
{
	//stop vdec channel.
    AW_MPI_VDEC_StopRecvStream(stContext.mVDecChn);
    AW_MPI_VDEC_DestroyChn(stContext.mVDecChn);
    //exit mpp system
    AW_MPI_SYS_Exit();
	stContext.mVDecChn = MM_INVALID_CHN;
    return NO_ERROR;
}

status_t CameraJpegDecoder::loadSrcFile(VDEC_STREAM_S *pStreamInfo, char *srcPath)		//test
{
	ifstream file(srcPath, ifstream::binary);
	if(!file) 
	{
		alogw("can't open jpeg file, exit loop!");
		return UNKNOWN_ERROR;
	}
	
	file.seekg(0, ios::end);
	pStreamInfo->mLen = file.tellg();
	file.seekg(0, ios::beg);
	vector<char> buffer(pStreamInfo->mLen);
    file.read(buffer.data(), pStreamInfo->mLen);
	pStreamInfo->pAddr = (unsigned char*) malloc(pStreamInfo->mLen);    //(unsigned char*) malloc(4096*1024);
	if(pStreamInfo->pAddr != NULL)
		copy(buffer.begin(), buffer.end(), pStreamInfo->pAddr);
    pStreamInfo->mbEndOfFrame = 1;
    pStreamInfo->mbEndOfStream = 0;
	file.close();	
	return NO_ERROR;
}

void CameraJpegDecoder::freeSrcFileBuf(VDEC_STREAM_S *pStreamInfo)		//test
{
	if(pStreamInfo->pAddr != NULL) {
		free(pStreamInfo->pAddr);
		pStreamInfo->pAddr = NULL;
	}
}

status_t CameraJpegDecoder::decode(VDEC_STREAM_S *pStreamInfo)
{
	int ret;
	
	//reopen ve for pic resolution may change
    ret = AW_MPI_VDEC_ReopenVideoEngine(stContext.mVDecChn);
    if (ret != SUCCESS)
    {
        aloge("reopen ve failed?!");
		return UNKNOWN_ERROR;
    }

    //send stream to vdec
    ret = AW_MPI_VDEC_SendStream(stContext.mVDecChn, pStreamInfo, 100);
    if(ret != SUCCESS)
    {
        alogw("send stream with 100ms timeout fail?!");
		return UNKNOWN_ERROR;
    }
	return NO_ERROR;
}

status_t CameraJpegDecoder::getFrame(VIDEO_FRAME_INFO_S *pFrameInfo)
{
    //get frame from vdec
    if (AW_MPI_VDEC_GetImage(stContext.mVDecChn, pFrameInfo, -1) != ERR_VDEC_NOBUF)
    {
        //AW_MPI_VDEC_ReleaseImage(stContext.mVDecChn, pFrameInfo);
    }
	else 
	{
		aloge("get frame fail!");
		return UNKNOWN_ERROR;
	}
	return NO_ERROR;
}

void CameraJpegDecoder::returnFrame(VIDEO_FRAME_INFO_S *pFrameInfo)
{
    AW_MPI_VDEC_ReleaseImage(stContext.mVDecChn, pFrameInfo);
}

void CameraJpegDecoder::getDataSize(VIDEO_FRAME_INFO_S *pFrameInfo, int *size_array)
{
	switch(pFrameInfo->VFrame.mPixelFormat)
	{
		case MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420:
			size_array[0] = pFrameInfo->VFrame.mWidth * pFrameInfo->VFrame.mHeight;
			size_array[1] = pFrameInfo->VFrame.mWidth * pFrameInfo->VFrame.mHeight / 2;
			size_array[2] = 0;
			break;
		default:
			size_array[0] = 0;
			size_array[1] = 0;
			size_array[2] = 0;
			break;
	}
}

}; /* namespace EyeseeLinux */
