/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <linux/netlink.h>
//#include <linux/kobject.h>
#include <linux/rtnetlink.h>
#include <errno.h>

#include "device_model/system/uevent_manager.h"
#include "common/thread.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "UeventManager"

#define UEVENT_BUFFER_SIZE	2048

using namespace std;

UeventManager::UeventManager()
{
db_debug("max+ UeventManager: 00");
	listen_fd_ = -1;
    listen_fd_ = UeventMonitorInit();
    if(listen_fd_ > 0)
        ThreadCreate(&uevent_monitor_thread_id_, NULL, UeventManager::EventLoopThread, this);
db_debug("max+ UeventManager: End, fd=%d", listen_fd_);
}

UeventManager::~UeventManager()
{
	if (uevent_monitor_thread_id_ > 0) 
		pthread_cancel(uevent_monitor_thread_id_);
	
	if(listen_fd_ >= 0) 
		close(listen_fd_);
}

int UeventManager::UeventMonitorInit() {
  const int buffersize = UEVENT_BUFFER_SIZE;
  int listen_fd;
  int ret;
  int on = 1;
  struct sockaddr_nl s_addr;
db_debug("max+ UeventMonitorInit: 00");  
  memset(&s_addr, 0, sizeof(s_addr));
  s_addr.nl_family = AF_NETLINK;
  s_addr.nl_pid = getpid();
  s_addr.nl_groups = 0xffffffff;

  listen_fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
  if(listen_fd < 0) {
    db_error("create socket failed: %s", strerror(errno));
    return -1;
  }
db_debug("max+ UeventMonitorInit: 01");   
  if(setsockopt(listen_fd, SOL_SOCKET, /*SO_RCVBUFFORCE*/ SO_RCVBUF, &buffersize, sizeof(buffersize)) < 0) {
    db_error("setsockopet SO_RCVBUF error\n");
	return -1;
  }
db_debug("max+ UeventMonitorInit: 02");   
  if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
	db_error("setsockopet SO_REUSEADDR error\n");
	return -1;
  }
db_debug("max+ UeventMonitorInit: 03"); 
  if(bind(listen_fd, (struct sockaddr *)&s_addr, sizeof(s_addr)) < 0) {
    db_error("bind socket failed");
    close(listen_fd);
    return -1;
  }
db_debug("max+ UeventMonitorInit: End"); 
  return listen_fd;
}

void *UeventManager::EventLoopThread(void *context) {
  char buf[UEVENT_BUFFER_SIZE * 2] = {0};
  int event;

  UeventManager *em = reinterpret_cast<UeventManager *>(context);

  db_msg("event loop");
  //prctl(PR_SET_NAME, "EventLoopThread", 0, 0, 0);

  while (1) {
	if(recv(em->listen_fd_, &buf, sizeof(buf),0) > 0)
	{
		//db_debug("%s\n", buf);
		//CUSBListener_onUSB(buf);
	}
	usleep(1000000);
  }

  return NULL;
}