/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/
 
 #include "Device/US363/Media/Camera/uvc.h"
 
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>          /* for videodev2.h */
#include <linux/videodev2.h>

#include "Device/US363/us360.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::UVC"

static char             dev_name[32];
int                     uvc_fd              = -1;
struct buffer *         buffers_p             = NULL;
static unsigned int     n_buffers           = 0;

int cameraId = 0;
int cameraBase = 0;

int checkCamerabase() {
    struct stat st;
    int i;
    int start_from_4 = 1;

    // if /dev/video[0-3] exist, camerabase=4, otherwise, camrerabase = 0
    for(i=0 ; i<4 ; i++){
        sprintf(dev_name,"/dev/video%d",i);
        if (-1 == stat (dev_name, &st)) {
            start_from_4 &= 0;
        }else{
            start_from_4 &= 1;
        }
    }

    if(start_from_4){
        return 4;
    }else{
        return 0;
    }
}

int opendevice(int i) {
    struct stat st;

    sprintf(dev_name,"/dev/video%d",i);

    if (-1 == stat (dev_name, &st)) {
        db_error("Cannot identify '%s': %d, %s\n", dev_name, errno, strerror (errno));
        return ERROR_LOCAL;
    }

    if (!S_ISCHR (st.st_mode)) {
        db_error("%s is no device\n", dev_name);
        return ERROR_LOCAL;
    }

    //fd = open (dev_name, O_RDWR | O_NONBLOCK, 0);
    uvc_fd = open (dev_name, O_RDWR);

    if (-1 == uvc_fd) {
        db_error("Cannot open '%s': %d, %s\n", dev_name, errno, strerror (errno));
        return ERROR_LOCAL;
    }
    return SUCCESS_LOCAL;
}

int initdevice() {
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    struct v4l2_fmtdesc enum_fmt;
    unsigned int min;

    if(uvc_fd == -1){
        db_error("initdevice: uvc_fd=-1\n");
        return ERROR_LOCAL;
    }
    if (-1 == xioctl (uvc_fd, VIDIOC_QUERYCAP, &cap)) {
        if (EINVAL == errno) {
            db_error("%s is no V4L2 device\n", dev_name);
            return ERROR_LOCAL;
        } else {
            return errnoexit ("VIDIOC_QUERYCAP");
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        db_error("%s is no video capture device\n", dev_name);
        return ERROR_LOCAL;
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        db_error("%s does not support streaming i/o\n", dev_name);
        return ERROR_LOCAL;
    }

    CLEAR (cropcap);

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (0 == xioctl (uvc_fd, VIDIOC_CROPCAP, &cropcap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect;

        if (-1 == xioctl (uvc_fd, VIDIOC_S_CROP, &crop)) {
            switch (errno) {
                case EINVAL:
                    break;
                default:
                    break;
            }
        }
    } else {
    }

    memset(&enum_fmt, 0, sizeof(enum_fmt));
    enum_fmt.index = 0;
    enum_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while (ioctl(uvc_fd, VIDIOC_ENUM_FMT, &enum_fmt) == 0)
    {
        enum_fmt.index++;
        db_debug("{ pixelformat = ''%c%c%c%c'', description = ''%s'' }\n",
                enum_fmt.pixelformat & 0xFF, (enum_fmt.pixelformat >> 8) & 0xFF,
                (enum_fmt.pixelformat >> 16) & 0xFF, (enum_fmt.pixelformat >> 24) & 0xFF,
                enum_fmt.description);
    }

    CLEAR (fmt);
    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = 6144;     //1920; //6144; //IMG_WIDTH;
    fmt.fmt.pix.height      = 1792;     //1080; //1792; //IMG_HEIGHT;
    fmt.fmt.pix.pixelformat = IMG_Pixelformat;
    fmt.fmt.pix.field       = V4L2_FIELD_NONE; //V4L2_FIELD_INTERLACED;

    if (-1 == xioctl (uvc_fd, VIDIOC_S_FMT, &fmt))
        return errnoexit ("VIDIOC_S_FMT");

    if (-1 == xioctl (uvc_fd, VIDIOC_G_FMT, &fmt))
        return errnoexit ("VIDIOC_G_FMT");

    //db_debug("w %d h %d pix '%c%c%c%c' field %d\n", fmt.fmt.pix.width, fmt.fmt.pix.height,
    //        fmt.fmt.pix.pixelformat & 0xFF, (fmt.fmt.pix.pixelformat >> 8) & 0xFF,
    //        (fmt.fmt.pix.pixelformat >> 16) & 0xFF, (fmt.fmt.pix.pixelformat >> 24) & 0xFF,
    //        fmt.fmt.pix.field);

    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;

    return initmmap ();
}

int initmmap() {
    struct v4l2_requestbuffers req;

    if(uvc_fd == -1){
        db_error("initmmap: uvc_fd=-1\n");
        return ERROR_LOCAL;
    }
    CLEAR (req);

    req.count               = 4;		//2;
    req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory              = V4L2_MEMORY_MMAP;

    if (-1 == xioctl (uvc_fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            db_error("%s does not support memory mapping\n", dev_name);
            return ERROR_LOCAL;
        } else {
            return errnoexit ("VIDIOC_REQBUFS");
        }
    }

    if (req.count < 2) {
        db_error("Insufficient buffer memory on %s\n", dev_name);
        return ERROR_LOCAL;
     }

    buffers_p = calloc (req.count, sizeof (*buffers_p));

    if (!buffers_p) {
        db_error("Out of memory\n");
        return ERROR_LOCAL;
    }

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        struct v4l2_buffer buf;

         CLEAR (buf);

        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = n_buffers;

        if (-1 == xioctl (uvc_fd, VIDIOC_QUERYBUF, &buf))
            return errnoexit ("VIDIOC_QUERYBUF");

        buffers_p[n_buffers].length = buf.length;
        buffers_p[n_buffers].start =
        mmap (NULL ,
            buf.length,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            uvc_fd, buf.m.offset);

        if (MAP_FAILED == buffers_p[n_buffers].start)
            return errnoexit ("mmap");
    }

    return SUCCESS_LOCAL;
}

void *Get_Buffer_Start(int idx) {
	return buffers_p[idx].start;
}

int startcapturing() {
    unsigned int i;
    enum v4l2_buf_type type;

    if(uvc_fd == -1){
        db_error("startcapturing: uvc_fd=-1\n");
        return ERROR_LOCAL;
    }
    for (i = 0; i < n_buffers; ++i) {
        struct v4l2_buffer buf;

        CLEAR (buf);

        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = i;

        if (-1 == xioctl (uvc_fd, VIDIOC_QBUF, &buf))
            return errnoexit ("VIDIOC_QBUF");
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (-1 == xioctl (uvc_fd, VIDIOC_STREAMON, &type))
        return errnoexit ("VIDIOC_STREAMON");

    return SUCCESS_LOCAL;
}

int stopcapturing() {
    enum v4l2_buf_type type;
    
    if(uvc_fd == -1){
        db_error("stopcapturing: uvc_fd=-1\n");
        return ERROR_LOCAL;
    }
    else db_debug("stopcapturing: uvc_fd=%d\n", uvc_fd);
    
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (-1 == xioctl (uvc_fd, VIDIOC_STREAMOFF, &type))
        return errnoexit ("VIDIOC_STREAMOFF");

    return SUCCESS_LOCAL;

}

int uninitdevice() {
    unsigned int i;
    if(buffers_p == NULL){
        db_error("uninitdevice: buffers_p=NULL\n");
        return ERROR_LOCAL;
    }
    else db_debug("uninitdevice: buffers_p=%x\n", buffers_p);
    
    for (i = 0; i < n_buffers; ++i) {
        if (-1 == munmap (buffers_p[i].start, buffers_p[i].length)) {
            db_error("uninitdevice() munmap err!\n");
            return errnoexit ("munmap");
        }
    }
    free (buffers_p);

    return SUCCESS_LOCAL;
}

int closedevice() {
    if(uvc_fd == -1){
        db_error("closedevice: uvc_fd=-1\n");
        return ERROR_LOCAL;
    }
    else db_debug("closedevice: uvc_fd=%d\n", uvc_fd);

    if (-1 == close (uvc_fd)){
        uvc_fd = -1;
        db_error("closedevice() close err!\n");
        return errnoexit ("close");
    }
    else
        uvc_fd = -1;
    return SUCCESS_LOCAL;
}

/*
 *     關閉 uvc driver
 */
void stopCamera() {
    stopcapturing ();
    uninitdevice ();
    closedevice ();
}

int xioctl(int fd, int request, void *arg) {
    int r;
    do r = ioctl (fd, request, arg);
    while (-1 == r && EINTR == errno);
    return r;
}

/*
 *    開啟uvc driver
 *    open driver = videoid + videobase
 *    ex: video0 = 0 + 0
 */
int openUVC(int videoid, int videobase) {
    int i, ret;

    for(i = 0; i < 5; i++) {
    	cameraBase = videobase;
        if(cameraBase < 0){
        	cameraBase = checkCamerabase();
        }

        ret = opendevice(cameraBase + videoid);
        db_debug("openUVC: opendevice~ i=%d ret=%d\n", i, ret);
        if(ret != ERROR_LOCAL){
            ret = initdevice();
            db_debug("openUVC: initdevice~ i=%d ret=%d\n", i, ret);
        }
        if(ret != ERROR_LOCAL){
            ret = startcapturing();
            db_debug("openUVC: startcapturing~ i=%d ret=%d\n", i, ret);
            if(ret != SUCCESS_LOCAL){
                stopCamera();
                db_debug("openUVC: device resetted\n");
            }
            else break;
        }
    }

    return ret;
}

int Check_UVC_Fd() {
	if(uvc_fd < 0)
		return -1;
	else
		return 0;
}

int UVC_Select() {
	fd_set fds;
    struct timeval tv;
    int r;

    FD_ZERO (&fds);
    FD_SET (uvc_fd, &fds);

    tv.tv_sec = 2;
    tv.tv_usec = 0;

    r = select (uvc_fd + 1, &fds, NULL, NULL, &tv);
    if(-1 == r)
      	db_error("select error\n");
	
	return r;
}

int UVC_DQBUF(struct v4l2_buffer *buf) {
	int ret;
    CLEAR (buf);
    buf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf->memory = V4L2_MEMORY_MMAP;

	ret = xioctl (uvc_fd, VIDIOC_DQBUF, buf);
    assert (buf->index < n_buffers);	
	return ret;
}

int UVC_QBUF(struct v4l2_buffer *buf) {
	int ret;
	ret = xioctl (uvc_fd, VIDIOC_QBUF, buf);
    return ret;
}

int Get_Camera_Id() {
	return cameraId;
}

int Get_Camera_Base() {
	return cameraBase;
}