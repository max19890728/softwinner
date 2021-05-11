/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/
#ifndef __UEVENT_MANAGER_H__
#define __UEVENT_MANAGER_H__

#include <linux/netlink.h>

class UeventManager
{
public:
    UeventManager();
    ~UeventManager();
	
	static UeventManager& instance() {
		static UeventManager instance_;
		return instance_;
	}

	
private:
	int UeventMonitorInit();
	static void *EventLoopThread(void *context);
	
	pthread_t uevent_monitor_thread_id_;
	int listen_fd_;
	int usb_status;
	
};

#endif /* __UEVENT_MANAGER_H__ */