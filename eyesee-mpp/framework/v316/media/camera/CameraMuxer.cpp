/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#define LOG_NDEBUG 0
#define LOG_TAG "CameraMuxer"
#include <utils/plat_log.h>

#include <cstdlib>
#include <cstring>
#include <mpi_mux.h>
#include <mpi_venc.h>
#include <mpi_sys.h>
#include <mm_common.h>
#include <mm_comm_venc.h>
#include <mm_comm_aenc.h>
#include <mm_comm_sys.h>
//#include <mm_comm_mux.h>
//#include "ion_memmanager.h"

#include "CameraMuxer.h"
//#include "CameraH264Encoder.h"


#define DEFAULT_SIMPLE_CACHE_SIZE_VFS       (64*1024)

using namespace std;
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

static int file_cnt = 0;

CameraMuxer::CameraMuxer()
{
    memset(&stMuxContext, 0, sizeof(stMuxContext));
}

CameraMuxer::~CameraMuxer()
{
	//
}

int CameraMuxer::getFileNameByCurTime(char *pNameBuf)
{
    int len = strlen(stMuxContext.mConfig.dstVideoFile);
    char *ptr = &stMuxContext.mConfig.dstVideoFile[0];
    while (*(ptr+len-1) != '.')
    {
        len--;
    }

    strncpy(pNameBuf, stMuxContext.mConfig.dstVideoFile, len-1);
    sprintf(pNameBuf, "%s_%d.mp4", pNameBuf, file_cnt);
    file_cnt++;
    alogd("file name: %s", pNameBuf);
    return 0;
}

ERRORTYPE CameraMuxer::MuxCallback(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData)
{
    ERRORTYPE ret = SUCCESS;
    CameraMuxer *pThiz = (CameraMuxer*)cookie;
	int muxerId;
    MuxContext *pMuxData = NULL;
	char fileName[128] = {0};
	
    if(MOD_ID_MUX == pChn->mModId)
    {
        switch(event)
        {
        case MPP_EVENT_RECORD_DONE:
            {
                muxerId = *(int*)pEventData;
                alogd("file done, mux_id=%d", muxerId);
            }
            break;

        case MPP_EVENT_NEED_NEXT_FD:
            {
                //muxerId = *(int*)pEventData;
                //pMuxData = (MuxContext *)&pThiz->stMuxContext;

                alogd("mux need next fd");
                //getFileNameByCurTime(&fileName[0]);
                //setOutputFileSync(pMuxData, &fileName[0], 0, muxerId);
            }
            break;

        case MPP_EVENT_BSFRAME_AVAILABLE:
            {
                alogd("mux bs frame available");
            }
            break;

        default:
            break;
        }
    }
    else
    {
        aloge("fatal error! why modId[0x%x]?!", pChn->mModId);
        ret = FAILURE;
    }
    return ret;
}

int CameraMuxer::getVideoSize(int width, int height)
{
	int size = 3840;
	if(width == 3840 && height == 1920)
		size = 3840;
	else if(width == 1920 && height == 1080)
		size = 1080;
	else if(width == 1280 && height == 720)
		size = 720;
	else if(width == 640 && height == 480)
		size = 480;
	return size;
}

ERRORTYPE CameraMuxer::initConfig(CameraMuxerConfig *pConfig)
{
	if(pConfig == NULL)
		return FAILURE;
	
    memset(pConfig, 0, sizeof(CameraMuxerConfig));
	
	//video
    pConfig->mVideoWidth  = 3840;
    pConfig->mVideoHeight = 1920;
	pConfig->mVideoSize   = getVideoSize(pConfig->mVideoWidth, pConfig->mVideoHeight);

    pConfig->mField           = VIDEO_FIELD_FRAME;
    pConfig->mVideoEncoderFmt = PT_H264;
    pConfig->mVideoFrameRate  = 10;
    pConfig->mVideoBitRate    = 10000000;
    pConfig->mEncUseProfile   = H264_PROFILE_HIGH;
    pConfig->mRcMode          = 0;
	
	//audio
	pConfig->mChannels        = 1;
    pConfig->mBitsPerSample   = 16; 
    pConfig->mSamplesPerFrame = 1024;
    pConfig->mSampleRate      = 44100;    
    pConfig->mAudioEncodeType = PT_AAC;
	
	//muxer
	snprintf(pConfig->dstVideoFile, MAX_FILE_PATH_LEN-1, "/mnt/extsd/video.mp4\0");
	pConfig->mMediaFileFmt    = MEDIA_FILE_FORMAT_MP4;
	pConfig->mMaxFileDuration = 60;

    return SUCCESS;
}

ERRORTYPE CameraMuxer::setConfig(CameraMuxerConfig *pConfig)
{
	if(pConfig == NULL)
		return FAILURE;
	
    memcpy(&stMuxContext.mConfig, pConfig, sizeof(stMuxContext.mConfig));

    stMuxContext.mMuxGrp = MM_INVALID_CHN;
    stMuxContext.mVeChn  = MM_INVALID_CHN;
	stMuxContext.mAeChn  = MM_INVALID_CHN;
    stMuxContext.mCurRecState = REC_NOT_PREPARED;

    return SUCCESS;
}

/*int CameraMuxer::vencBufMgrCreate(int frmNum, int videoSize, INPUT_BUF_Q *pBufList, BOOL isAwAfbc)
{
    unsigned int width, height, size;
    unsigned int afbc_header;
	
	if(pBufList == NULL) 
		return -1;

    if (videoSize == 720)
    {
        width = 1280;
        height = 720;
        size = 1280*720;
    }
    else if (videoSize == 1080)
    {
        width = 1920;
        height = 1080;
        size = 1920*1080;
    }
    else if (videoSize == 3840)
    {
        width = 3840;
        height = 2160;
        size = 3840*2160;
    }
    else
    {
        aloge("not support this video size:%d p", videoSize);
        return -1;
    }

    INIT_LIST_HEAD(&pBufList->mIdleList);
    INIT_LIST_HEAD(&pBufList->mReadyList);
    INIT_LIST_HEAD(&pBufList->mUseList);

    pthread_mutex_init(&pBufList->mIdleListLock, NULL);
    pthread_mutex_init(&pBufList->mReadyListLock, NULL);
    pthread_mutex_init(&pBufList->mUseListLock, NULL);

    if (isAwAfbc)
    {
        afbc_header = ((width +127)>>7)*((height+31)>>5)*96;
    }

    int i;
    for (i = 0; i < frmNum; ++i)
    {
        IN_FRAME_NODE_S *pFrameNode = (IN_FRAME_NODE_S*)malloc(sizeof(IN_FRAME_NODE_S));
        if (pFrameNode == NULL)
        {
            aloge("alloc IN_FRAME_NODE_S error!");
            break;
        }
        memset(pFrameNode, 0, sizeof(IN_FRAME_NODE_S));

        if (!isAwAfbc)
        {
            pFrameNode->mFrame.VFrame.mpVirAddr[0] = (void *)venc_allocMem(size);
            if (pFrameNode->mFrame.VFrame.mpVirAddr[0] == NULL)
            {
                aloge("alloc y_vir_addr size %d error!", size);
                free(pFrameNode);
                break;
            }
            pFrameNode->mFrame.VFrame.mpVirAddr[1] = (void *)venc_allocMem(size/2);
            if (pFrameNode->mFrame.VFrame.mpVirAddr[1] == NULL)
            {
                aloge("alloc uv_vir_addr size %d error!", size/2);
                venc_freeMem(pFrameNode->mFrame.VFrame.mpVirAddr[0]);
                pFrameNode->mFrame.VFrame.mpVirAddr[0] = NULL;
                free(pFrameNode);
                break;
            }

            pFrameNode->mFrame.VFrame.mPhyAddr[0] = (unsigned int)venc_getPhyAddrByVirAddr(pFrameNode->mFrame.VFrame.mpVirAddr[0]);
            pFrameNode->mFrame.VFrame.mPhyAddr[1] = (unsigned int)venc_getPhyAddrByVirAddr(pFrameNode->mFrame.VFrame.mpVirAddr[1]);
        }
        else
        {
            pFrameNode->mFrame.VFrame.mpVirAddr[0] = (void *)venc_allocMem(size+size/2+afbc_header);
            if (pFrameNode->mFrame.VFrame.mpVirAddr[0] == NULL)
            {
                aloge("fail to alloc aw_afbc y_vir_addr!");
                free(pFrameNode);
                break;
            }
            pFrameNode->mFrame.VFrame.mPhyAddr[0] = (unsigned int)venc_getPhyAddrByVirAddr(pFrameNode->mFrame.VFrame.mpVirAddr[0]);
        }

        pFrameNode->mFrame.mId = i;
        list_add_tail(&pFrameNode->mList, &pBufList->mIdleList);
        pBufList->mFrameNum++;
    }

    if (pBufList->mFrameNum == 0)
    {
        aloge("alloc no node!!");
        return -1;
    }
    else
    {
        return 0;
    }
}*/

/*int CameraMuxer::vencBufMgrDestroy(INPUT_BUF_Q *pBufList)
{
    IN_FRAME_NODE_S *pEntry, *pTmp;
    int frmnum = 0;

    if (pBufList == NULL)
    {
        aloge("pBufList null");
        return -1;
    }

    pthread_mutex_lock(&pBufList->mUseListLock);
    if (!list_empty(&pBufList->mUseList))
    {
        aloge("error! SendingFrmList should be 0! maybe some frames not release!");
        list_for_each_entry_safe(pEntry, pTmp, &pBufList->mUseList, mList)
        {
            list_del(&pEntry->mList);
            venc_freeMem(pEntry->mFrame.VFrame.mpVirAddr[0]);
            if (NULL != pEntry->mFrame.VFrame.mpVirAddr[1])
            {
                venc_freeMem(pEntry->mFrame.VFrame.mpVirAddr[1]);
            }
            free(pEntry);
            ++frmnum;
        }
    }
    pthread_mutex_unlock(&pBufList->mUseListLock);

    pthread_mutex_lock(&pBufList->mReadyListLock);
    if (!list_empty(&pBufList->mReadyList))
    {
        list_for_each_entry_safe(pEntry, pTmp, &pBufList->mReadyList, mList)
        {
            list_del(&pEntry->mList);
            venc_freeMem(pEntry->mFrame.VFrame.mpVirAddr[0]);
            if (NULL != pEntry->mFrame.VFrame.mpVirAddr[1])
            {
                venc_freeMem(pEntry->mFrame.VFrame.mpVirAddr[1]);
            }
            free(pEntry);
            ++frmnum;
        }
    }
    pthread_mutex_unlock(&pBufList->mReadyListLock);


    pthread_mutex_lock(&pBufList->mIdleListLock);
    if (!list_empty(&pBufList->mIdleList))
    {
        list_for_each_entry_safe(pEntry, pTmp, &pBufList->mIdleList, mList)
        {
            list_del(&pEntry->mList);
            venc_freeMem(pEntry->mFrame.VFrame.mpVirAddr[0]);
            if (NULL != pEntry->mFrame.VFrame.mpVirAddr[1])
            {
                venc_freeMem(pEntry->mFrame.VFrame.mpVirAddr[1]);
            }
            free(pEntry);
            ++frmnum;
        }
    }
    pthread_mutex_unlock(&pBufList->mIdleListLock);

    if (frmnum != pBufList->mFrameNum)
    {
        aloge("Fatal error! frame node number is not match[%d][%d]", frmnum, pBufList->mFrameNum);
    }

    pthread_mutex_destroy(&pBufList->mIdleListLock);
    pthread_mutex_destroy(&pBufList->mReadyListLock);
    pthread_mutex_destroy(&pBufList->mUseListLock);

    return 0;
}*/

ERRORTYPE CameraMuxer::configMuxGrpAttr(MuxContext *pContext)
{
	if(pContext == NULL) 
		return FAILURE;
	
    memset(&pContext->mMuxGrpAttr, 0, sizeof(MUX_GRP_ATTR_S));

    pContext->mMuxGrpAttr.mVideoEncodeType = pContext->mConfig.mVideoEncoderFmt;
    pContext->mMuxGrpAttr.mWidth           = pContext->mConfig.mVideoWidth;
    pContext->mMuxGrpAttr.mHeight          = pContext->mConfig.mVideoHeight;
    pContext->mMuxGrpAttr.mVideoFrmRate    = pContext->mConfig.mVideoFrameRate*1000;
    //pContext->mMuxGrpAttr.mMaxKeyInterval = 
	
	pContext->mMuxGrpAttr.mChannels        = pContext->mConfig.mChannels;
    pContext->mMuxGrpAttr.mBitsPerSample   = pContext->mConfig.mBitsPerSample; 
    pContext->mMuxGrpAttr.mSamplesPerFrame = pContext->mConfig.mSamplesPerFrame;
    pContext->mMuxGrpAttr.mSampleRate      = pContext->mConfig.mSampleRate;    
    pContext->mMuxGrpAttr.mAudioEncodeType = pContext->mConfig.mAudioEncodeType;

    return SUCCESS;
}

ERRORTYPE CameraMuxer::createMuxGrp(MuxContext *pContext)
{
    ERRORTYPE ret;
    BOOL nSuccessFlag = FALSE;
	
	if(pContext == NULL) 
		return FAILURE;

    configMuxGrpAttr(pContext);
    pContext->mMuxGrp = 0;
    while (pContext->mMuxGrp < MUX_MAX_GRP_NUM)
    {
        ret = AW_MPI_MUX_CreateGrp(pContext->mMuxGrp, &pContext->mMuxGrpAttr);
        if (SUCCESS == ret)
        {
            nSuccessFlag = TRUE;
            alogd("create mux group[%d] success!", pContext->mMuxGrp);
            break;
        }
        else if (ERR_MUX_EXIST == ret)
        {
            alogd("mux group[%d] is exist, find next!", pContext->mMuxGrp);
            pContext->mMuxGrp++;
        }
        else
        {
            alogd("create mux group[%d] ret[0x%x], find next!", pContext->mMuxGrp, ret);
            pContext->mMuxGrp++;
        }
    }

    if (FALSE == nSuccessFlag)
    {
        pContext->mMuxGrp = MM_INVALID_CHN;
        aloge("fatal error! create mux group fail!");
        return FAILURE;
    }
    else
    {
        MPPCallbackInfo cbInfo;
        cbInfo.cookie = (void*)this;
        cbInfo.callback = (MPPCallbackFuncType)&MuxCallback;
        AW_MPI_MUX_RegisterCallback(pContext->mMuxGrp, &cbInfo);
        return SUCCESS;
    }
}

int CameraMuxer::addOutputFormatAndOutputSink_1(MuxContext *pContext, OUTSINKINFO_S *pSinkInfo)
{
    int retMuxerId = -1;
	
	if(pContext == NULL) 
		return -1;

    alogd("fmt:0x%x, fd:%d, FallocateLen:%d, callback_out_flag:%d", pSinkInfo->mOutputFormat, pSinkInfo->mOutputFd, pSinkInfo->mFallocateLen, pSinkInfo->mCallbackOutFlag);
    if (pSinkInfo->mOutputFd >= 0 && TRUE == pSinkInfo->mCallbackOutFlag)
    {
        aloge("fatal error! one muxer cannot support two sink methods!");
        return -1;
    }

    //find if the same output_format sinkInfo exist or callback out stream is exist.
	if(!pContext->mMuxChnList.empty())
    {
		for(std::vector<MUX_CHN_INFO_S>::iterator it = pContext->mMuxChnList.begin(); it != pContext->mMuxChnList.end(); ++it)
		{
			if (it->mSinkInfo.mOutputFormat == pSinkInfo->mOutputFormat)
            {
                alogd("Be careful! same outputForamt[0x%x] exist in array", pSinkInfo->mOutputFormat);
            }
            if (it->mSinkInfo.mCallbackOutFlag == pSinkInfo->mCallbackOutFlag)
            {
                aloge("fatal error! only support one callback out stream");
            }
		}
    }

    MUX_CHN_INFO_S p_node;;
    memset(&p_node, 0, sizeof(MUX_CHN_INFO_S));
    p_node.mSinkInfo.mMuxerId = pContext->mMuxerIdCounter;
    p_node.mSinkInfo.mOutputFormat = pSinkInfo->mOutputFormat;
    if (pSinkInfo->mOutputFd > 0)
    {
        p_node.mSinkInfo.mOutputFd = dup(pSinkInfo->mOutputFd);
    }
    else
    {
        p_node.mSinkInfo.mOutputFd = -1;
    }
    p_node.mSinkInfo.mFallocateLen = pSinkInfo->mFallocateLen;
    p_node.mSinkInfo.mCallbackOutFlag = pSinkInfo->mCallbackOutFlag;

    p_node.mMuxChnAttr.mMuxerId = p_node.mSinkInfo.mMuxerId;
    p_node.mMuxChnAttr.mMediaFileFormat = p_node.mSinkInfo.mOutputFormat;
    p_node.mMuxChnAttr.mMaxFileDuration = pContext->mConfig.mMaxFileDuration * 1000; //s -> ms
    p_node.mMuxChnAttr.mFallocateLen = p_node.mSinkInfo.mFallocateLen;
    p_node.mMuxChnAttr.mCallbackOutFlag = p_node.mSinkInfo.mCallbackOutFlag;
    p_node.mMuxChnAttr.mFsWriteMode = FSWRITEMODE_SIMPLECACHE;
    p_node.mMuxChnAttr.mSimpleCacheSize = DEFAULT_SIMPLE_CACHE_SIZE_VFS;

    p_node.mMuxChn = MM_INVALID_CHN;

    if ((pContext->mCurRecState == REC_PREPARED) || (pContext->mCurRecState == REC_RECORDING))
    {
        ERRORTYPE ret;
        BOOL nSuccessFlag = FALSE;
        MUX_CHN nMuxChn = 0;
        while (nMuxChn < MUX_MAX_CHN_NUM)
        {
            ret = AW_MPI_MUX_CreateChn(pContext->mMuxGrp, nMuxChn, &p_node.mMuxChnAttr, p_node.mSinkInfo.mOutputFd);
            if (SUCCESS == ret)
            {
                nSuccessFlag = TRUE;
                alogd("create mux group[%d] channel[%d] success, muxerId[%d]!", pContext->mMuxGrp, nMuxChn, p_node.mMuxChnAttr.mMuxerId);
                break;
            }
            else if (ERR_MUX_EXIST == ret)
            {
                alogd("mux group[%d] channel[%d] is exist, find next!", pContext->mMuxGrp, nMuxChn);
                nMuxChn++;
            }
            else
            {
                aloge("fatal error! create mux group[%d] channel[%d] fail ret[0x%x], find next!", pContext->mMuxGrp, nMuxChn, ret);
                nMuxChn++;
            }
        }

        if (nSuccessFlag)
        {
            retMuxerId = p_node.mSinkInfo.mMuxerId;
            p_node.mMuxChn = nMuxChn;
            pContext->mMuxerIdCounter++;
        }
        else
        {
            aloge("fatal error! create mux group[%d] channel fail!", pContext->mMuxGrp);
            if (p_node.mSinkInfo.mOutputFd >= 0)
            {
                close(p_node.mSinkInfo.mOutputFd);
                p_node.mSinkInfo.mOutputFd = -1;
            }

            retMuxerId = -1;
        }

		pContext->mMuxChnList.push_back(p_node);
    }
    else
    {
        retMuxerId = p_node.mSinkInfo.mMuxerId;
        pContext->mMuxerIdCounter++;
		pContext->mMuxChnList.push_back(p_node);
    }

    return retMuxerId;
}

int CameraMuxer::addOutputFormatAndOutputSink(MuxContext *pContext, char* path, MEDIA_FILE_FORMAT_E mediaFileFmt)
{
    int muxerId = -1;
    OUTSINKINFO_S sinkInfo = {0};
	
	if(pContext == NULL) 
		return -1;

    if (path != NULL)
    {
        sinkInfo.mFallocateLen = 0;
        sinkInfo.mCallbackOutFlag = FALSE;
        sinkInfo.mOutputFormat = mediaFileFmt;
        sinkInfo.mOutputFd = open(path, O_RDWR | O_CREAT, 0666);
        if (sinkInfo.mOutputFd < 0)
        {
            aloge("Failed to open %s", path);
            return -1;
        }

        muxerId = addOutputFormatAndOutputSink_1(pContext, &sinkInfo);
        close(sinkInfo.mOutputFd);
    }

    return muxerId;
}

int CameraMuxer::setOutputFileSync_1(MuxContext *pContext, int fd, int64_t fallocateLength, int muxerId)
{
	if(pContext == NULL) 
		return -1;

    alogv("setOutputFileSync fd=%d", fd);
    if (fd < 0)
    {
        aloge("Invalid parameter");
        return -1;
    }

    MUX_CHN muxChn = MM_INVALID_CHN;
	if(!pContext->mMuxChnList.empty())
    {
		for(std::vector<MUX_CHN_INFO_S>::iterator it = pContext->mMuxChnList.begin(); it != pContext->mMuxChnList.end(); ++it)
		{
            if (it->mMuxChnAttr.mMuxerId == muxerId)
            {
                muxChn = it->mMuxChn;
                break;
            }
		}
    }

    if (muxChn != MM_INVALID_CHN)
    {
        AW_MPI_MUX_SwitchFd(pContext->mMuxGrp, muxChn, fd, fallocateLength);
        return 0;
    }
    else
    {
        aloge("fatal error! can't find muxChn which muxerId[%d]", muxerId);
        return -1;
    }
}

int CameraMuxer::setOutputFileSync(MuxContext *pContext, char* path, int64_t fallocateLength, int muxerId)
{
    int ret;
	
	if(pContext == NULL) 
		return -1;

    if (pContext->mCurRecState != REC_RECORDING)
    {
        aloge("not in recording state");
        return -1;
    }

    if (path != NULL)
    {
        int fd = open(path, O_RDWR | O_CREAT, 0666);
        if (fd < 0)
        {
            aloge("fail to open %s", path);
            return -1;
        }
        ret = setOutputFileSync_1(pContext, fd, fallocateLength, muxerId);
        close(fd);

        return ret;
    }
    else
    {
        return -1;
    }
}

int CameraMuxer::configVencChnAttr(MuxContext *pContext)
{
	if(pContext == NULL) 
		return FAILURE;
	
    memset(&pContext->mVencChnAttr, 0, sizeof(VENC_CHN_ATTR_S));

    pContext->mVencChnAttr.VeAttr.Type = pContext->mConfig.mVideoEncoderFmt;
    //pContext->mVencChnAttr.VeAttr.MaxKeyInterval = pContext->mConfig.mVideoMaxKeyItl;

    pContext->mVencChnAttr.VeAttr.SrcPicWidth  = pContext->mConfig.mVideoWidth;
    pContext->mVencChnAttr.VeAttr.SrcPicHeight = pContext->mConfig.mVideoHeight;

    pContext->mVencChnAttr.VeAttr.Field = VIDEO_FIELD_FRAME;

    if (PT_H264 == pContext->mVencChnAttr.VeAttr.Type)
    {
        pContext->mVencChnAttr.VeAttr.AttrH264e.bByFrame = TRUE;
        //pContext->mVencChnAttr.VeAttr.AttrH264e.Profile = VENC_H264ProfileHigh;
        pContext->mVencChnAttr.VeAttr.AttrH264e.Profile = pContext->mConfig.mEncUseProfile;
        pContext->mVencChnAttr.VeAttr.AttrH264e.PicWidth  = pContext->mConfig.mVideoWidth;
        pContext->mVencChnAttr.VeAttr.AttrH264e.PicHeight = pContext->mConfig.mVideoHeight;
        switch (pContext->mConfig.mRcMode)
        {
        case 1:
            pContext->mVencChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H264VBR;
            pContext->mVencChnAttr.RcAttr.mAttrH264Vbr.mMinQp = 10;
            pContext->mVencChnAttr.RcAttr.mAttrH264Vbr.mMaxQp = 40;
            break;
        case 2:
            pContext->mVencChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H264FIXQP;
            pContext->mVencChnAttr.RcAttr.mAttrH264FixQp.mIQp = 35;
            pContext->mVencChnAttr.RcAttr.mAttrH264FixQp.mPQp = 35;
            break;
        case 3:
            pContext->mVencChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H264QPMAP;
            break;
        case 0:
        default:
            pContext->mVencChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H264CBR;
            pContext->mVencChnAttr.RcAttr.mAttrH264Cbr.mSrcFrmRate = pContext->mConfig.mVideoFrameRate;
            pContext->mVencChnAttr.RcAttr.mAttrH264Cbr.fr32DstFrmRate = pContext->mConfig.mVideoFrameRate;
            pContext->mVencChnAttr.RcAttr.mAttrH264Cbr.mBitRate = pContext->mConfig.mVideoBitRate;
            break;
        }
    }
    else if (PT_H265 == pContext->mVencChnAttr.VeAttr.Type)
    {
        pContext->mVencChnAttr.VeAttr.AttrH265e.mbByFrame = TRUE;
        pContext->mVencChnAttr.VeAttr.AttrH265e.mProfile = pContext->mConfig.mEncUseProfile;
        pContext->mVencChnAttr.VeAttr.AttrH265e.mPicWidth = pContext->mConfig.mVideoWidth;
        pContext->mVencChnAttr.VeAttr.AttrH265e.mPicHeight = pContext->mConfig.mVideoHeight;
        switch (pContext->mConfig.mRcMode)
        {
        case 1:
            pContext->mVencChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H265VBR;
            pContext->mVencChnAttr.RcAttr.mAttrH265Vbr.mMinQp = 10;
            pContext->mVencChnAttr.RcAttr.mAttrH265Vbr.mMaxQp = 40;
            break;
        case 2:
            pContext->mVencChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H265FIXQP;
            pContext->mVencChnAttr.RcAttr.mAttrH265FixQp.mIQp = 35;
            pContext->mVencChnAttr.RcAttr.mAttrH265FixQp.mPQp = 35;
            break;
        case 3:
            pContext->mVencChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H265QPMAP;
            break;
        case 0:
        default:
            pContext->mVencChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H265CBR;
            pContext->mVencChnAttr.RcAttr.mAttrH265Cbr.mSrcFrmRate = pContext->mConfig.mVideoFrameRate;
            pContext->mVencChnAttr.RcAttr.mAttrH265Cbr.fr32DstFrmRate = pContext->mConfig.mVideoFrameRate;
            pContext->mVencChnAttr.RcAttr.mAttrH265Cbr.mBitRate = pContext->mConfig.mVideoBitRate;
            break;
        }
    }
    else if (PT_MJPEG == pContext->mVencChnAttr.VeAttr.Type)
    {
        pContext->mVencChnAttr.VeAttr.AttrMjpeg.mbByFrame = TRUE;
        pContext->mVencChnAttr.VeAttr.AttrMjpeg.mPicWidth = pContext->mConfig.mVideoWidth;
        pContext->mVencChnAttr.VeAttr.AttrMjpeg.mPicHeight = pContext->mConfig.mVideoHeight;
        pContext->mVencChnAttr.RcAttr.mRcMode = VENC_RC_MODE_MJPEGCBR;
        pContext->mVencChnAttr.RcAttr.mAttrMjpegeCbr.mBitRate = pContext->mConfig.mVideoBitRate;
    }

    return SUCCESS;
}

ERRORTYPE CameraMuxer::createVencChn(MuxContext *pContext, VENC_CHN vChn)
{
	if(pContext == NULL) 
		return FAILURE;
	
    configVencChnAttr(pContext);
    pContext->mVeChn = vChn;
    return SUCCESS;
}

int CameraMuxer::configAencChnAttr(MuxContext *pContext)
{
	if(pContext == NULL) 
		return FAILURE;
	
    memset(&pContext->mAencChnAttr, 0, sizeof(AENC_CHN_ATTR_S));

    pContext->mAencChnAttr.AeAttr.Type          = pContext->mConfig.mAudioEncodeType;
	pContext->mAencChnAttr.AeAttr.sampleRate    = pContext->mConfig.mSampleRate;
	pContext->mAencChnAttr.AeAttr.channels      = pContext->mConfig.mChannels;
	pContext->mAencChnAttr.AeAttr.bitRate       = pContext->mConfig.mSampleRate;
	pContext->mAencChnAttr.AeAttr.bitsPerSample = pContext->mConfig.mBitsPerSample;
	//pContext->mAencChnAttr.AeAttr.attachAACHeader = ;

    return SUCCESS;
}

ERRORTYPE CameraMuxer::createAencChn(MuxContext *pContext, AENC_CHN aChn)
{
	if(pContext == NULL) 
		return FAILURE;
	
    configAencChnAttr(pContext);
    pContext->mAeChn = aChn;
    return SUCCESS;
}

ERRORTYPE CameraMuxer::prepare(MuxContext *pContext, VENC_CHN vChn, AENC_CHN aChn)
{
    BOOL nSuccessFlag;
    MUX_CHN nMuxChn;
    ERRORTYPE ret;
    ERRORTYPE result = SUCCESS;
	VencHeaderData H264SpsPpsInfo;
	VencHeaderData H265SpsPpsInfo;
	
	if(pContext == NULL) 
		return FAILURE;

    if (createVencChn(pContext, vChn) != SUCCESS)
    {
        aloge("create venc chn fail");
        return FAILURE;
    }
	
	if (createAencChn(pContext, aChn) != SUCCESS)
    {
        aloge("create aenc chn fail");
        return FAILURE;
    }

    if (createMuxGrp(pContext) != SUCCESS)
    {
        aloge("create mux group fail");
        return FAILURE;
    }

    //set spspps
    if (pContext->mConfig.mVideoEncoderFmt == PT_H264)
    {
        AW_MPI_VENC_GetH264SpsPpsInfo(pContext->mVeChn, &H264SpsPpsInfo);
        AW_MPI_MUX_SetH264SpsPpsInfo(pContext->mMuxGrp, &H264SpsPpsInfo);
    }
    else if(pContext->mConfig.mVideoEncoderFmt == PT_H265)
    {
        AW_MPI_VENC_GetH265SpsPpsInfo(pContext->mVeChn, &H265SpsPpsInfo);
        AW_MPI_MUX_SetH265SpsPpsInfo(pContext->mMuxGrp, &H265SpsPpsInfo);
    }

	if(!pContext->mMuxChnList.empty())
    {
		for(std::vector<MUX_CHN_INFO_S>::iterator it = pContext->mMuxChnList.begin(); it != pContext->mMuxChnList.end(); ++it)
		{
            nMuxChn = 0;
            nSuccessFlag = FALSE;
            while (it->mMuxChn < MUX_MAX_CHN_NUM)
            {
                ret = AW_MPI_MUX_CreateChn(pContext->mMuxGrp, nMuxChn, &it->mMuxChnAttr, it->mSinkInfo.mOutputFd);
                if (SUCCESS == ret)
                {
                    nSuccessFlag = TRUE;
                    alogd("create mux group[%d] channel[%d] success, muxerId[%d]!", pContext->mMuxGrp, it->mMuxChn, it->mMuxChnAttr.mMuxerId);
                    break;
                }
                else if(ERR_MUX_EXIST == ret)
                {
                    nMuxChn++;
                    //break;
                }
                else
                {
                    nMuxChn++;
                }
            }

            if (FALSE == nSuccessFlag)
            {
                result = FAILURE;
                it->mMuxChn = MM_INVALID_CHN;
                aloge("fatal error! create mux group[%d] channel fail!", pContext->mMuxGrp);
            }
            else
            {
                it->mMuxChn = nMuxChn;
            }
		}
    }
    else
    {
        aloge("maybe something wrong,chn list is empty");
    }

    if (pContext->mMuxGrp >= 0)
    {
        MPP_CHN_S MuxGrp = {MOD_ID_MUX , 0, pContext->mMuxGrp};
		MPP_CHN_S VeChn  = {MOD_ID_VENC, 0, pContext->mVeChn};
		MPP_CHN_S AeChn  = {MOD_ID_AENC, 0, pContext->mAeChn};
		
		if(pContext->mVeChn >= 0) 
		{
			AW_MPI_SYS_Bind(&VeChn, &MuxGrp);
		}
		
		if(pContext->mAeChn >= 0) 
		{
			AW_MPI_SYS_Bind(&AeChn, &MuxGrp);
		}
		
        pContext->mCurRecState = REC_PREPARED;
    }

    return result;
}

ERRORTYPE CameraMuxer::start()
{
    ERRORTYPE ret = SUCCESS;

    if (stMuxContext.mMuxGrp >= 0)
    {
        AW_MPI_MUX_StartGrp(stMuxContext.mMuxGrp);
    }

    stMuxContext.mCurRecState = REC_RECORDING;

    return ret;
}

ERRORTYPE CameraMuxer::stop()
{
    ERRORTYPE ret = SUCCESS;
	
    alogd("stop");
    stMuxContext.mCurRecState = REC_STOP;

    if (stMuxContext.mMuxGrp >= 0)
    {
        alogd("stop mux grp");
        AW_MPI_MUX_StopGrp(stMuxContext.mMuxGrp);
    }

    if (stMuxContext.mMuxGrp >= 0)
    {
        alogd("destory mux grp");
        AW_MPI_MUX_DestroyGrp(stMuxContext.mMuxGrp);
        stMuxContext.mMuxGrp = MM_INVALID_CHN;
    }

    return ret;
}

status_t CameraMuxer::initialize(CameraMuxerConfig *pConfig, VENC_CHN vChn, AENC_CHN aChn)
{
	if(pConfig == NULL) 
		return UNKNOWN_ERROR;
	
	setConfig(pConfig);

	stMuxContext.mSysConf.nAlignWidth = 32;
    AW_MPI_SYS_SetConf(&stMuxContext.mSysConf);
    AW_MPI_SYS_Init();

	addOutputFormatAndOutputSink(&stMuxContext, stMuxContext.mConfig.dstVideoFile, stMuxContext.mConfig.mMediaFileFmt);

    if (prepare(&stMuxContext, vChn, aChn) != SUCCESS)
    {
        aloge("prepare fail!");
        goto err_out_4;
    }
	
	start();

	return NO_ERROR;

err_out_4:
	if(!stMuxContext.mMuxChnList.empty())
    {
        alogd("free chn list node");
		for(std::vector<MUX_CHN_INFO_S>::iterator it = stMuxContext.mMuxChnList.begin(); it != stMuxContext.mMuxChnList.end(); ++it)
		{
            if (it->mSinkInfo.mOutputFd > 0)
            {
                alogd("close file");
                close(it->mSinkInfo.mOutputFd);
                it->mSinkInfo.mOutputFd = -1;
            }
			//stMuxContext.mMuxChnList.erase(it);
        }
		stMuxContext.mMuxChnList.clear();
    }

    AW_MPI_SYS_Exit();
	
	return UNKNOWN_ERROR;
}

status_t CameraMuxer::destroy()
{
	stop();
	
	if(!stMuxContext.mMuxChnList.empty())
    {
        alogd("free chn list node");
		for(std::vector<MUX_CHN_INFO_S>::iterator it = stMuxContext.mMuxChnList.begin(); it != stMuxContext.mMuxChnList.end(); ++it)
		{
            if (it->mSinkInfo.mOutputFd > 0)
            {
                alogd("close file");
                close(it->mSinkInfo.mOutputFd);
                it->mSinkInfo.mOutputFd = -1;
            }
			//stMuxContext.mMuxChnList.erase(it);
        }
		stMuxContext.mMuxChnList.clear();
    }

    //AW_MPI_SYS_Exit();
	
    return NO_ERROR;
}

}; /* namespace EyeseeLinux */
