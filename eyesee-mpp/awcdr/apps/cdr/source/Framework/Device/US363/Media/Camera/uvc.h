/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __UVC_H__
#define __UVC_H__

#include <linux/videodev2.h>

#ifdef __cplusplus
extern "C" {
#endif

struct buffer {
        void *                  start;
        size_t                  length;
};

int checkCamerabase();
int opendevice(int i);
int initdevice();
int startcapturing();
int stopcapturing();
int uninitdevice();
int closedevice();
void stopCamera();
int xioctl(int fd, int request, void *arg);
int openUVC(int videoid, int videobase);
int Check_UVC_Fd();
int UVC_Select();
int UVC_DQBUF(struct v4l2_buffer *buf);
int UVC_QBUF(struct v4l2_buffer *buf);
void *Get_Buffer_Start(int idx);
int Get_Camera_Id();
int Get_Camera_Base();

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__UVC_H__