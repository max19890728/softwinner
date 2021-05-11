/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __LIVE360_THREAD_H__
#define __LIVE360_THREAD_H__

#ifdef __cplusplus
extern "C" {
#endif

void live360_cmd_queue_init();
void set_live360_t1();
void get_live360_t1(unsigned long long *time);
void set_live360_thread_en(int en);
void set_live360_state(int en);
int get_live360_state();
void set_live360_cmd(int cmd);
int get_live360_cmd();
void live360_thread_init();

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__LIVE360_THREAD_H__