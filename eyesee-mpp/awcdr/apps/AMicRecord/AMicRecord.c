//#define LOG_NDEBUG 0
#define LOG_TAG "sample_virvi2vo"
#include <utils/plat_log.h>

#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include <confparser.h>
#include "AMicRecord_config.h"
#include "AMicRecord.h"

AMicRecordContext *gpAMicRecordContext = NULL;

typedef enum
{
    AMicRecord_Exit = 0,
} AMicRecordMessage;

int initAMicRecordContext(AMicRecordContext *pContext)
{
    memset(pContext, 0, sizeof(AMicRecordContext));
    message_create(&pContext->mMessageQueue);
    return 0;
}

int destroyAMicRecordContext(AMicRecordContext *pContext)
{
    message_destroy(&pContext->mMessageQueue);
    return 0;
}

AMicRecordContext* constructAMicRecordContext()
{
    AMicRecordContext *pContext = (AMicRecordContext*)malloc(sizeof(AMicRecordContext));
    if(pContext != NULL)
    {
        initAMicRecordContext(pContext);
    }
    return pContext;
}
void destructAMicRecordContext(AMicRecordContext *pContext)
{
    if(pContext)
    {
        destroyAMicRecordContext(pContext);
        free(pContext);
        pContext = NULL;
    }
}

static int ParseCmdLine(int argc, char **argv, AMicRecordCmdLineParam *pCmdLinePara)
{
    alogd("AMicRecord path:[%s], arg number is [%d]", argv[0], argc);
    int ret = 0;
    int i=1;
    memset(pCmdLinePara, 0, sizeof(AMicRecordCmdLineParam));
    while(i < argc)
    {
        if(!strcmp(argv[i], "-path"))
        {
            if(++i >= argc)
            {
                aloge("fatal error! use -h to learn how to set parameter!!!");
                ret = -1;
                break;
            }
            if(strlen(argv[i]) >= MAX_FILE_PATH_SIZE)
            {
                aloge("fatal error! file path[%s] too long: [%d]>=[%d]!", argv[i], strlen(argv[i]), MAX_FILE_PATH_SIZE);
            }
            strncpy(pCmdLinePara->mConfigFilePath, argv[i], MAX_FILE_PATH_SIZE-1);
            pCmdLinePara->mConfigFilePath[MAX_FILE_PATH_SIZE-1] = '\0';
        }
        else if(!strcmp(argv[i], "-h"))
        {
            printf("CmdLine param example:\n"
                "\t run -path /home/AMicRecord.conf\n");
            ret = 1;
            break;
        }
        else
        {
            alogd("ignore invalid CmdLine param:[%s], type -h to get how to set parameter!", argv[i]);
        }
        i++;
    }
    return ret;
}

static ERRORTYPE loadAMicRecordConfig(AMicRecordConfig *pConfig, const char *conf_path)
{
    int ret;
    char *ptr;
    if(NULL == conf_path)
    {
        alogd("user not set config file. use default test parameter!");
        strcpy(pConfig->mDirPath, "/mnt/extsd/AMicRecord_Files");
        strcpy(pConfig->mFileName, "ai.pcm");
        pConfig->mSampleRate = 16000;
        pConfig->mChannelCount = 2;
        pConfig->mBitWidth = 16;
        pConfig->mCapureDuration = 0;
        pConfig->mbSaveWav = 0;
        return SUCCESS;
    }
    CONFPARSER_S stConfParser;
    ret = createConfParser(conf_path, &stConfParser);
    if(ret < 0)
    {
        aloge("load conf fail");
        return FAILURE;
    }
    memset(pConfig, 0, sizeof(AMicRecordConfig));
    char *pStrDirPath = (char*)GetConfParaString(&stConfParser, AMICRECORD_KEY_PCM_DIR_PATH, NULL);
    if(pStrDirPath != NULL)
    {
        strncpy(pConfig->mDirPath, pStrDirPath, MAX_FILE_PATH_SIZE);
    }
    char *pStrFileName = (char*)GetConfParaString(&stConfParser, AMICRECORD_KEY_PCM_FILE_NAME, NULL);
    if(pStrFileName != NULL)
    {
        strncpy(pConfig->mFileName, pStrFileName, MAX_FILE_PATH_SIZE);
    }
    pConfig->mSampleRate = GetConfParaInt(&stConfParser, AMICRECORD_KEY_PCM_SAMPLE_RATE, 0);
    pConfig->mChannelCount = GetConfParaInt(&stConfParser, AMICRECORD_KEY_PCM_CHANNEL_CNT, 0);
    pConfig->mBitWidth = GetConfParaInt(&stConfParser, AMICRECORD_KEY_PCM_BIT_WIDTH, 0);
    pConfig->mCapureDuration = GetConfParaInt(&stConfParser, AMICRECORD_KEY_PCM_CAP_DURATION, 0);
    pConfig->mbSaveWav = GetConfParaInt(&stConfParser, AMICRECORD_KEY_SAVE_WAV, 0);
    alogd("dirPath[%s], fileName[%s], sampleRate[%d], channelCount[%d], bitWidth[%d]", pConfig->mDirPath, pConfig->mFileName, pConfig->mSampleRate, pConfig->mChannelCount, pConfig->mBitWidth);
    alogd("captureDuration[%d]s, bSaveWav[%d]", pConfig->mCapureDuration, pConfig->mbSaveWav);
    destroyConfParser(&stConfParser);
    return SUCCESS;
}

void handle_exit(int signo)
{
    alogd("user want to exit!");
    if(NULL != gpAMicRecordContext)
    {
        message_t msg;
        msg.command = AMicRecord_Exit;
        put_message(&gpAMicRecordContext->mMessageQueue, &msg);
    }
}

static void config_AIO_ATTR_S_by_AMicRecordConfig(AIO_ATTR_S *pAttr, AMicRecordConfig *pConfig)
{
    memset(pAttr, 0, sizeof(AIO_ATTR_S));
    pAttr->enSamplerate = (AUDIO_SAMPLE_RATE_E)pConfig->mSampleRate;
    pAttr->enBitwidth = (AUDIO_BIT_WIDTH_E)(pConfig->mBitWidth/8-1);
    pAttr->enWorkmode = AIO_MODE_I2S_MASTER;
    if(pConfig->mChannelCount > 1)
    {
        pAttr->enSoundmode = AUDIO_SOUND_MODE_STEREO;
    }
    else
    {
        pAttr->enSoundmode = AUDIO_SOUND_MODE_MONO;
    }
    pAttr->u32ChnCnt = pConfig->mChannelCount;
    pAttr->u32ClkSel = 0;
    pAttr->mPcmCardId = PCM_CARD_TYPE_AUDIOCODEC;
}

int AMicRecord_CreateDirectory(const char* strDir)
{
    if(NULL == strDir)
    {
        aloge("fatal error! Null string pointer!");
        return -1;
    }
    //check folder existence
    struct stat sb;
    if (stat(strDir, &sb) == 0)
    {
        if(S_ISDIR(sb.st_mode))
        {
            return 0;
        }
        else
        {
            aloge("fatal error! [%s] is exist, but mode[0x%x] is not directory!", strDir, sb.st_mode);
            return -1;
        }
    }
    //create folder if necessary
    int ret = mkdir(strDir, S_IRWXU | S_IRWXG | S_IRWXO);
    if(!ret)
    {
        alogd("create folder[%s] success", strDir);
        return 0;
    }
    else
    {
        aloge("fatal error! create folder[%s] failed!", strDir);
        return -1;
    }
}

int main(int argc __attribute__((__unused__)), char *argv[] __attribute__((__unused__)))
{
    int result = 0;
	printf("AMicRecord running!\n");	
    AMicRecordContext *pContext = constructAMicRecordContext();
    gpAMicRecordContext = pContext;
    if(NULL == pContext)
    {
        printf("why construct fail?\n");
        return -1;
    }
    /* parse command line param */
    if(ParseCmdLine(argc, argv, &pContext->mCmdLinePara) != 0)
    {
        //aloge("fatal error! command line param is wrong, exit!");
        result = -1;
        goto _err_1;
    }
    char *pConfigFilePath;
    if(strlen(pContext->mCmdLinePara.mConfigFilePath) > 0)
    {
        pConfigFilePath = pContext->mCmdLinePara.mConfigFilePath;
    }
    else
    {
        pConfigFilePath = NULL;
    }

    /* parse config file. */
    if(loadAMicRecordConfig(&pContext->mConfigPara, pConfigFilePath) != SUCCESS)
    {
        aloge("fatal error! no config file or parse conf file fail");
        result = -1;
        goto _err_1;
    }
    //check folder existence, create folder if necessary
    if(0 != AMicRecord_CreateDirectory(pContext->mConfigPara.mDirPath))
    {
        result = -1;
        goto _err_1;
    }
    /* register process function for SIGINT, to exit program. */
    if (signal(SIGINT, handle_exit) == SIG_ERR)
        perror("can't catch SIGSEGV");

    ERRORTYPE ret;
    /* init mpp system */
    memset(&pContext->mSysConf, 0, sizeof(MPP_SYS_CONF_S));
    pContext->mSysConf.nAlignWidth = 32;
    AW_MPI_SYS_SetConf(&pContext->mSysConf);
    ret = AW_MPI_SYS_Init();
    if (ret != SUCCESS) 
    {
        aloge("fatal error! AW_MPI_SYS_Init failed");
        goto _err_1;
    }
    //create SavePcmFile.
    char strFilePath[MAX_FILE_PATH_SIZE];
    snprintf(strFilePath, MAX_FILE_PATH_SIZE, "%s/%s", pContext->mConfigPara.mDirPath, pContext->mConfigPara.mFileName);
    int nBufferDuration = 30;
    int nBufferSize = pContext->mConfigPara.mChannelCount*(pContext->mConfigPara.mBitWidth/8)*pContext->mConfigPara.mSampleRate*nBufferDuration;
    pContext->mpSavePcmFile = constructSavePcmFile(strFilePath, nBufferSize, pContext->mConfigPara.mbSaveWav, pContext->mConfigPara.mSampleRate, pContext->mConfigPara.mChannelCount, pContext->mConfigPara.mBitWidth);
    if(NULL == pContext->mpSavePcmFile)
    {
        aloge("fatal error! construct save pcm file fail!");
    }
    //config ai attr
    pContext->mAIDev = 0;
    config_AIO_ATTR_S_by_AMicRecordConfig(&pContext->mAIOAttr, &pContext->mConfigPara);
    AW_MPI_AI_SetPubAttr(pContext->mAIDev, &pContext->mAIOAttr);
    alogd("AI Attr is: sampleRate[%d] bitWidth[%d], workMode[%d], Soundmode[%d], ChnCnt[%d], CldSel[%d], pcmCardId[%d]", 
        pContext->mAIOAttr.enSamplerate,
        pContext->mAIOAttr.enBitwidth,
        pContext->mAIOAttr.enWorkmode,
        pContext->mAIOAttr.enSoundmode,
        pContext->mAIOAttr.u32ChnCnt,
        pContext->mAIOAttr.u32ClkSel,
        pContext->mAIOAttr.mPcmCardId);
    //create ai channel.
    BOOL bSuccessFlag = FALSE;
    pContext->mAIChn = 0;
    while(pContext->mAIChn < AIO_MAX_CHN_NUM)
    {
        ret = AW_MPI_AI_CreateChn(pContext->mAIDev, pContext->mAIChn);
        if(SUCCESS == ret)
        {
            bSuccessFlag = TRUE;
            alogd("create ai channel[%d] success!", pContext->mAIChn);
            break;
        }
        else if (ERR_AI_EXIST == ret)
        {
            alogd("ai channel[%d] exist, find next!", pContext->mAIChn);
            pContext->mAIChn++;
        }
        else if(ERR_AI_NOT_ENABLED == ret)
        {
            aloge("fatal error! audio_hw_ai not started!");
            break;
        }
        else
        {
            aloge("fatal error! create ai channel[%d] fail! ret[0x%x]!", pContext->mAIChn, ret);
            break;
        }
    }
    if(FALSE == bSuccessFlag)
    {
        pContext->mAIChn = MM_INVALID_CHN;
        aloge("fatal error! create ai channel fail!");
        goto _err_2;
    }

    //start ai dev.
    ret = AW_MPI_AI_EnableChn(pContext->mAIDev, pContext->mAIChn);
    if(ret != SUCCESS)
    {
        aloge("fatal error! enable ai channel fail!");
    }

    int nSavePcmFileRet;
    AUDIO_FRAME_S stAFrame;
    memset(&stAFrame, 0, sizeof(AUDIO_FRAME_S));
    message_t stMsg;
    while (1)
    {
    PROCESS_MESSAGE:
        if(get_message(&pContext->mMessageQueue, &stMsg) == 0) 
        {
            alogv("AMicRecord thread get message cmd:%d", stMsg.command);
            if (AMicRecord_Exit == stMsg.command)
            {
                alogd("receive usr exit message!");
                break;
            }
            else
            {
                aloge("fatal error! unknown cmd[0x%x]", stMsg.command);
            }
            //precede to process message
            goto PROCESS_MESSAGE;
        }
        if(0 == stAFrame.mLen)
        {
            ret = AW_MPI_AI_GetFrame(pContext->mAIDev, pContext->mAIChn, &stAFrame, NULL, 2000);
            if(SUCCESS == ret)
            {
                pContext->mPcmSize += stAFrame.mLen;
                pContext->mPcmDurationMs = (int64_t)pContext->mPcmSize*1000/(pContext->mConfigPara.mChannelCount*(pContext->mConfigPara.mBitWidth/8)*pContext->mConfigPara.mSampleRate);
            }
        }
        else
        {
            alogd("Be careful! audio frame is not processed?");
            ret = SUCCESS;
        }
        if (SUCCESS == ret)
        {
        _sendPcmData:
            nSavePcmFileRet = pContext->mpSavePcmFile->SendPcmData((void*)pContext->mpSavePcmFile, stAFrame.mpAddr, stAFrame.mLen, 500);
            if(nSavePcmFileRet != 0)
            {
                if(ETIMEDOUT == nSavePcmFileRet)
                {
                    alogd("send pcm data timeout? continue to send!");
                    if(get_message_count(&pContext->mMessageQueue) > 0)
                    {
                        alogd("receive usr message2!");
                        goto PROCESS_MESSAGE;
                    }
                    goto _sendPcmData;
                }
                else
                {
                    aloge("send pcm data fail, ignore pcm data.");
                }
            }
            ret = AW_MPI_AI_ReleaseFrame(pContext->mAIDev, pContext->mAIChn, &stAFrame, NULL);
            if (SUCCESS != ret)
            {
                aloge("fatal error! release frame to ai fail! ret: %#x", ret);
            }
            memset(&stAFrame, 0, sizeof(AUDIO_FRAME_S));
            if(pContext->mConfigPara.mCapureDuration != 0)
            {
                if(pContext->mPcmDurationMs/1000 >= pContext->mConfigPara.mCapureDuration)
                {
                    alogd("time is over! exit!");
                    goto _exit;
                }
            }
        }
        else if(ERR_AI_BUF_EMPTY == ret)
        {
            alogd("timeout? continue!");
        }
        else
        {
            aloge("get pcm from ai in block mode fail! ret: %#x", ret);
            break;
        }
    }
_exit:
    //stop, reset and destroy ai chn & dev.
    AW_MPI_AI_DisableChn(pContext->mAIDev, pContext->mAIChn);
    AW_MPI_AI_ResetChn(pContext->mAIDev, pContext->mAIChn);
    AW_MPI_AI_DestroyChn(pContext->mAIDev, pContext->mAIChn);
    pContext->mAIChn = MM_INVALID_CHN;
    pContext->mAIDev = MM_INVALID_DEV;
    /* exit mpp system */
    AW_MPI_SYS_Exit();

    if(pContext->mpSavePcmFile)
    {
        destructSavePcmFile(pContext->mpSavePcmFile);
        pContext->mpSavePcmFile = NULL;
    }

    destructAMicRecordContext(pContext);
    pContext = NULL;
    gpAMicRecordContext = NULL;
    printf("AMicRecord exit success!\n");
    return 0;

_err_2:
    if(pContext->mpSavePcmFile)
    {
        destructSavePcmFile(pContext->mpSavePcmFile);
        pContext->mpSavePcmFile = NULL;
    }
    /* exit mpp system */
    AW_MPI_SYS_Exit();
_err_1:
    destroyAMicRecordContext(pContext);
    printf("AMicRecord exit!\n");
    return result;
}

