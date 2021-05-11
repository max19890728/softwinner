/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#define LOG_NDEBUG 0
#define LOG_TAG "CameraH264Encoder"
#include <utils/plat_log.h>

#include <cstdlib>
#include <cstring>
#include <mpi_venc.h>
#include <mpi_sys.h>
#include <mm_comm_venc.h>
#include "ion_memmanager.h"

#include "CameraH264Encoder.h"


using namespace std;
namespace EyeseeLinux {

CameraH264Encoder::CameraH264Encoder()
{
    memset(&stContext, 0, sizeof(stContext));
}

CameraH264Encoder::~CameraH264Encoder()
{
	//
}

ERRORTYPE CameraH264Encoder::VEncCallback(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData)
{
    ERRORTYPE ret = SUCCESS;
    CameraH264Encoder *pThiz = (CameraH264Encoder*)cookie;
	VIDEO_FRAME_INFO_S *pFrame = (VIDEO_FRAME_INFO_S *)pEventData;
	
    if(MOD_ID_VENC == pChn->mModId)
    {
        if(pChn->mChnId != pThiz->stContext.mVeChn)
        {
            aloge("fatal error! VEnc chnId[%d]!=[%d]", pChn->mChnId, pThiz->stContext.mVeChn);
        }
        switch(event)
        {
			case MPP_EVENT_RELEASE_VIDEO_BUFFER:
				if (pFrame != NULL)
				{
					/*pthread_mutex_lock(&pVencPara->mInBuf_Q.mUseListLock);
					if (!list_empty(&pVencPara->mInBuf_Q.mUseList))
					{
						IN_FRAME_NODE_S *pEntry, *pTmp;
						list_for_each_entry_safe(pEntry, pTmp, &pVencPara->mInBuf_Q.mUseList, mList)
						{
							if (pEntry->mFrame.mId == pFrame->mId)
							{
								pthread_mutex_lock(&pVencPara->mInBuf_Q.mIdleListLock);
								list_move_tail(&pEntry->mList, &pVencPara->mInBuf_Q.mIdleList);
								pthread_mutex_unlock(&pVencPara->mInBuf_Q.mIdleListLock);
								break;
							}
						}
					}
					pthread_mutex_unlock(&pVencPara->mInBuf_Q.mUseListLock);*/
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

int CameraH264Encoder::getVideoSize(int width, int height)
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

ERRORTYPE CameraH264Encoder::initConfig(CameraH264EncConfig *pConfig)
{
	if(pConfig == NULL) 
		return FAILURE;
	
	memset(pConfig, 0, sizeof(CameraH264EncConfig));
	
	pConfig->srcWidth  = 3840;
	pConfig->srcHeight = 1920;
	pConfig->srcSize   = getVideoSize(pConfig->srcWidth, pConfig->srcHeight);
    pConfig->srcPixFmt = MM_PIXEL_FORMAT_YUV_PLANAR_420;

	pConfig->dstWidth  = 3840;
	pConfig->dstHeight = 1920;
	pConfig->dstSize   = getVideoSize(pConfig->dstWidth, pConfig->dstHeight);
	
	pConfig->mVideoEncoderFmt = PT_H264;
    pConfig->mEncUseProfile   = H264_PROFILE_HIGH;		//Base/Main/High Profile
    pConfig->mVideoFrameRate  = 10;						//fps
	pConfig->mVideoMaxKeyItl  = 8;
    pConfig->mVideoBitRate    = 10000000;				//bitrate
    pConfig->mRcMode          = 0;						//VBR/CBR ...
    pConfig->mQp0             = 10;
    pConfig->mQp1             = 40;
    pConfig->rotate           = ROTATE_NONE;
	
    return SUCCESS;
}

ERRORTYPE CameraH264Encoder::setConfig(CameraH264EncConfig *pConfig)
{
	if(pConfig == NULL) 
		return FAILURE;

	memcpy(&stContext.mConfig, pConfig, sizeof(stContext.mConfig));
    return SUCCESS;
}

/*int CameraH264Encoder::vencBufMgrCreate(int frmNum, int videoSize, INPUT_BUF_Q *pBufList, BOOL isAwAfbc)
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

/*int CameraH264Encoder::vencBufMgrDestroy(INPUT_BUF_Q *pBufList)
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

ERRORTYPE CameraH264Encoder::configVencChnAttr(VEncContext *pContext)
{
	if(pContext == NULL) 
		return FAILURE;
	
    memset(&pContext->mVencChnAttr, 0, sizeof(VENC_CHN_ATTR_S));

    pContext->mVencChnAttr.VeAttr.Type = pContext->mConfig.mVideoEncoderFmt;
    pContext->mVencChnAttr.VeAttr.MaxKeyInterval = pContext->mConfig.mVideoMaxKeyItl;

    pContext->mVencChnAttr.VeAttr.SrcPicWidth  = pContext->mConfig.srcWidth;
    pContext->mVencChnAttr.VeAttr.SrcPicHeight = pContext->mConfig.srcHeight;
    pContext->mVencChnAttr.VeAttr.Rotate = pContext->mConfig.rotate;

    //pContext->mVencChnAttr.VeAttr.PixelFormat = pContext->mConfigPara.dstPixFmt;
    if (pContext->mConfig.srcPixFmt == MM_PIXEL_FORMAT_YUV_AW_AFBC)
    {
        alogd("aw_afbc");
        pContext->mVencChnAttr.VeAttr.PixelFormat = MM_PIXEL_FORMAT_YUV_AW_AFBC;
        pContext->mConfig.dstWidth = pContext->mConfig.srcWidth;  //cannot compress_encoder
        pContext->mConfig.dstHeight = pContext->mConfig.srcHeight; //cannot compress_encoder
    }
    else
    {
        pContext->mVencChnAttr.VeAttr.PixelFormat = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    }
    pContext->mVencChnAttr.VeAttr.Field = VIDEO_FIELD_FRAME;

    if (PT_H264 == pContext->mVencChnAttr.VeAttr.Type)
    {
        pContext->mVencChnAttr.VeAttr.AttrH264e.bByFrame = TRUE;
        //pContext->mVencChnAttr.VeAttr.AttrH264e.Profile = VENC_H264ProfileHigh;
        pContext->mVencChnAttr.VeAttr.AttrH264e.Profile = pContext->mConfig.mEncUseProfile;
        pContext->mVencChnAttr.VeAttr.AttrH264e.PicWidth  = pContext->mConfig.dstWidth;
        pContext->mVencChnAttr.VeAttr.AttrH264e.PicHeight = pContext->mConfig.dstHeight;
        switch (pContext->mConfig.mRcMode)
        {
        case 1:
            pContext->mVencChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H264VBR;
            pContext->mVencChnAttr.RcAttr.mAttrH264Vbr.mMinQp = pContext->mConfig.mQp0; //10;
            pContext->mVencChnAttr.RcAttr.mAttrH264Vbr.mMaxQp = pContext->mConfig.mQp1; //40;
            break;
        case 2:
            pContext->mVencChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H264FIXQP;
            pContext->mVencChnAttr.RcAttr.mAttrH264FixQp.mIQp = pContext->mConfig.mQp0;//35;
            pContext->mVencChnAttr.RcAttr.mAttrH264FixQp.mPQp = pContext->mConfig.mQp1;//35;
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
            pContext->mVencChnAttr.RcAttr.mAttrH264Cbr.mMinQp = pContext->mConfig.mQp0;
            pContext->mVencChnAttr.RcAttr.mAttrH264Cbr.mMaxQp = pContext->mConfig.mQp1;
            break;
        }
    }
    else if (PT_H265 == pContext->mVencChnAttr.VeAttr.Type)
    {
        pContext->mVencChnAttr.VeAttr.AttrH265e.mbByFrame = TRUE;
        pContext->mVencChnAttr.VeAttr.AttrH265e.mProfile = H265_PROFILE_STI11;
        pContext->mVencChnAttr.VeAttr.AttrH265e.mPicWidth = pContext->mConfig.dstWidth;
        pContext->mVencChnAttr.VeAttr.AttrH265e.mPicHeight = pContext->mConfig.dstHeight;
        switch (pContext->mConfig.mRcMode)
        {
        case 1:
            pContext->mVencChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H265VBR;
            pContext->mVencChnAttr.RcAttr.mAttrH265Vbr.mMinQp = pContext->mConfig.mQp0; //10;
            pContext->mVencChnAttr.RcAttr.mAttrH265Vbr.mMaxQp = pContext->mConfig.mQp1; //40;
            break;
        case 2:
            pContext->mVencChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H265FIXQP;
            pContext->mVencChnAttr.RcAttr.mAttrH265FixQp.mIQp = pContext->mConfig.mQp0; //35;
            pContext->mVencChnAttr.RcAttr.mAttrH265FixQp.mPQp = pContext->mConfig.mQp1; //35;
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
            pContext->mVencChnAttr.RcAttr.mAttrH265Cbr.mMinQp = pContext->mConfig.mQp0;
            pContext->mVencChnAttr.RcAttr.mAttrH265Cbr.mMaxQp = pContext->mConfig.mQp1;
            break;
        }
    }
    else if (PT_MJPEG == pContext->mVencChnAttr.VeAttr.Type)
    {
        pContext->mVencChnAttr.VeAttr.AttrMjpeg.mbByFrame = TRUE;
        pContext->mVencChnAttr.VeAttr.AttrMjpeg.mPicWidth = pContext->mConfig.dstWidth;
        pContext->mVencChnAttr.VeAttr.AttrMjpeg.mPicHeight = pContext->mConfig.dstHeight;
        pContext->mVencChnAttr.RcAttr.mRcMode = VENC_RC_MODE_MJPEGCBR;
        pContext->mVencChnAttr.RcAttr.mAttrMjpegeCbr.mBitRate = pContext->mConfig.mVideoBitRate;
    }

    return SUCCESS;
}

ERRORTYPE CameraH264Encoder::createVencChn(VEncContext *pContext)
{
    ERRORTYPE ret;
    BOOL nSuccessFlag = FALSE;
	
	if(pContext == NULL) 
		return FAILURE;

    configVencChnAttr(pContext);
    pContext->mVeChn = 0;
    while(pContext->mVeChn < VENC_MAX_CHN_NUM)
    {
        ret = AW_MPI_VENC_CreateChn(pContext->mVeChn, &pContext->mVencChnAttr);
        if (SUCCESS == ret)
        {
            nSuccessFlag = TRUE;
            alogd("create venc channel[%d] success!", pContext->mVeChn);
            break;
        }
        else if(ERR_VENC_EXIST == ret)
        {
            alogd("venc channel[%d] is exist, find next!", pContext->mVeChn);
            pContext->mVeChn++;
        }
        else
        {
            alogd("create venc channel[%d] ret[0x%x], find next!", pContext->mVeChn, ret);
            pContext->mVeChn++;
        }
    }

    if (!nSuccessFlag)
    {
        pContext->mVeChn = MM_INVALID_CHN;
        aloge("fatal error! create venc channel fail!");
        return FAILURE;
    }
    else
    {
        VENC_FRAME_RATE_S stFrameRate;
        stFrameRate.SrcFrmRate = stFrameRate.DstFrmRate = pContext->mConfig.mVideoFrameRate;
        AW_MPI_VENC_SetFrameRate(pContext->mVeChn, &stFrameRate);

        if (PT_MJPEG == pContext->mVencChnAttr.VeAttr.Type)
        {
            VENC_PARAM_JPEG_S stJpegParam;
            memset(&stJpegParam, 0, sizeof(VENC_PARAM_JPEG_S));
            stJpegParam.Qfactor = pContext->mConfig.mQp0;
            AW_MPI_VENC_SetJpegParam(pContext->mVeChn, &stJpegParam);
        }

        MPPCallbackInfo cbInfo;
        cbInfo.cookie = (void*)this;
        cbInfo.callback = (MPPCallbackFuncType)&VEncCallback;
        AW_MPI_VENC_RegisterCallback(pContext->mVeChn, &cbInfo);
        return SUCCESS;
    }
}

status_t CameraH264Encoder::initialize(CameraH264EncConfig *pConfig)
{
	int ret, size;
	
	if(pConfig == NULL) 
		return UNKNOWN_ERROR;
	
	setConfig(pConfig);

    if (ion_memOpen() != 0 )
    {
        alogd("open mem fail!");
        goto err_out_0;
    }

//    BOOL isAwAfbc = (MM_PIXEL_FORMAT_YUV_AW_AFBC == stContext.mConfig.srcPixFmt);
//    ret = vencBufMgrCreate(IN_FRAME_BUF_NUM, stContext.mConfig.srcSize, &stContext.mInBuf_Q, isAwAfbc);
//    if (ret != 0)
//    {
//        aloge("ERROR: create FrameBuf manager fail");
//        goto err_out_1;
//    }

//    size = stContext.mConfig.srcWidth * stContext.mConfig.srcHeight / 2; //for yuv420p -> yuv420sp tmp_buf
//    stContext.tmpBuf = (char *)malloc(size);
//    if (stContext.tmpBuf == NULL)
//    {
//		aloge("malloc tmpBuf fail");
//        goto err_out_2;
//    }

    stContext.mSysConf.nAlignWidth = 32;
    AW_MPI_SYS_SetConf(&stContext.mSysConf);
    AW_MPI_SYS_Init();

    if (createVencChn(&stContext) != SUCCESS)
    {
        aloge("create vec chn fail");
        goto err_out_3;
    }

    AW_MPI_VENC_StartRecvPic(stContext.mVeChn);
	
	return NO_ERROR;
	
err_out_3:
    AW_MPI_SYS_Exit();
//    if (stContext.tmpBuf != NULL)
//    {
//        free(stContext.tmpBuf);
//        stContext.tmpBuf = NULL;
//    }
//err_out_2:
//    vencBufMgrDestroy(&stContext.mInBuf_Q);
err_out_1:
    ion_memClose();	
err_out_0:	
	return UNKNOWN_ERROR;
}

status_t CameraH264Encoder::destroy()
{
	AW_MPI_VENC_StopRecvPic(stContext.mVeChn);
    if (stContext.mVeChn >= 0)
    {
        AW_MPI_VENC_ResetChn(stContext.mVeChn);
        AW_MPI_VENC_DestroyChn(stContext.mVeChn);
        stContext.mVeChn = MM_INVALID_CHN;
    }

    AW_MPI_SYS_Exit();
//    if (stContext.tmpBuf != NULL)
//    {
//        free(stContext.tmpBuf);
//        stContext.tmpBuf = NULL;
//    }

//    vencBufMgrDestroy(&stContext.mInBuf_Q);
    ion_memClose();
    return NO_ERROR;
}

status_t CameraH264Encoder::encode(VIDEO_FRAME_INFO_S *pFrameInfo)
{
	int ret;
	
	if(pFrameInfo == NULL) 
		return UNKNOWN_ERROR;
	
    ret = AW_MPI_VENC_SendFrame(stContext.mVeChn, pFrameInfo, 0);
    if (ret != SUCCESS)
    {
        alogd("send frame[%d] fail", pFrameInfo->mId);
		return UNKNOWN_ERROR;
    }
	
	return NO_ERROR;
}

void CameraH264Encoder::getSpsPpsHead(VencHeaderData *spspps)
{
	if(spspps == NULL) 
		return;
	
	memset(spspps, 0, sizeof(VencHeaderData));
    if (stContext.mConfig.mVideoEncoderFmt == PT_H264)
    {
        AW_MPI_VENC_GetH264SpsPpsInfo(stContext.mVeChn, spspps);
        //if (spspps->nLength)
        //{
        //    fwrite(spspps->pBuffer, 1, spspps->nLength, stContext.mConfig.fd_out);
        //}
    }
    else if (stContext.mConfig.mVideoEncoderFmt == PT_H265)
    {
        AW_MPI_VENC_GetH265SpsPpsInfo(stContext.mVeChn, spspps);
        //if (spspps->nLength)
        //{
        //    fwrite(spspps->pBuffer, 1, spspps->nLength, stContext.mConfig.fd_out);
        //}
    }
}

int CameraH264Encoder::getH264SpsPps(VencHeaderData src, H264_SPS_PPS_S *dst)
{
	int i, sps_flag = 0, pps_flag = 0;
	unsigned char *sps_p, *pps_p;
	
	if(dst == NULL) return -1;

	sps_p = src.pBuffer;
	pps_p = src.pBuffer;
	for(i = 0; i < src.nLength; i++) {
		if(sps_flag == 0) {
			if(*sps_p == 0x00 && *(sps_p+1) == 0x00 && *(sps_p+2) == 0x00 && *(sps_p+3) == 0x01 && *(sps_p+4) == 0x67)
				sps_flag = 1;
			else
				sps_p++;
		}

		if(pps_flag == 0) {
			if(*pps_p == 0x00 && *(pps_p+1) == 0x00 && *(pps_p+2) == 0x00 && *(pps_p+3) == 0x01 && *(pps_p+4) == 0x68)
				pps_flag = 1;
			else			
				pps_p++;
		}
		
		if(sps_flag == 1 && pps_flag == 1)
			break;
	}
	
	if(sps_flag != 1 || pps_flag != 1)
		return -1;
	
	if(pps_p >= sps_p) {
		dst->sps_len = (pps_p - sps_p - 4);
		dst->pps_len = (src.pBuffer + src.nLength - pps_p - 4);
	}
	else {
		dst->sps_len = (src.pBuffer + src.nLength - sps_p - 4);
		dst->pps_len = (sps_p - pps_p - 4);
	}
	memcpy(&dst->sps[0], (sps_p+4), dst->sps_len);
	memcpy(&dst->pps[0], (pps_p+4), dst->pps_len);
	
	return 0;
}

status_t CameraH264Encoder::getFrame(VENC_STREAM_S *pStream)
{
	if(pStream == NULL) 
		return UNKNOWN_ERROR;
	
	//VENC_PACK_S mpPack;
	//memset(pStream, 0, sizeof(VENC_STREAM_S));
    //pStream->mpPack = &mpPack;
    //pStream->mPackCount = 1;
	
    if (AW_MPI_VENC_GetStream(stContext.mVeChn, pStream, 100) == SUCCESS)
    {
        /*if (pStream->mpPack->mLen0)
        {
            fwrite(pStream->mpPack->mpAddr0, 1, pStream->mpPack->mLen0, stContext.mConfig.fd_out);
        }

        if (pStream->mpPack->mLen1)
        {
            fwrite(pStream->mpPack->mpAddr1, 1, pStream->mpPack->mLen1, stContext.mConfig.fd_out);
        }*/
        //alogd("get pts=%llu, seq=%d", pStream->mpPack->mPTS, pStream->mSeq);
        //AW_MPI_VENC_ReleaseStream(stContext.mVeChn, pStream);
    }
	else
		return UNKNOWN_ERROR;

	return NO_ERROR;
}

void CameraH264Encoder::returnFrame(VENC_STREAM_S *pStream)
{
	if(pStream == NULL) 
		return;
	AW_MPI_VENC_ReleaseStream(stContext.mVeChn, pStream);
}

int CameraH264Encoder::getDataSize(VENC_STREAM_S *pStream)
{
	return (pStream->mpPack->mLen0 + pStream->mpPack->mLen1 + pStream->mpPack->mLen2);
}

int CameraH264Encoder::getData(VENC_STREAM_S *pStream, unsigned char *pData)
{
	unsigned char *ptr = pData;
	if(pStream->mpPack->mLen0 > 0) {
		memcpy(ptr, pStream->mpPack->mpAddr0, pStream->mpPack->mLen0);
		ptr += pStream->mpPack->mLen0;
	}
	if(pStream->mpPack->mLen1 > 0) {
		memcpy(ptr, pStream->mpPack->mpAddr1, pStream->mpPack->mLen1);
		ptr += pStream->mpPack->mLen1;
	}
	if(pStream->mpPack->mLen2 > 0) {
		memcpy(ptr, pStream->mpPack->mpAddr2, pStream->mpPack->mLen2);
		ptr += pStream->mpPack->mLen2;
	}
	return getDataSize(pStream);
}	

}; /* namespace EyeseeLinux */
