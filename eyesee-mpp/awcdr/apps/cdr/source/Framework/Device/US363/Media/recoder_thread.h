/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __RECODER_THREAD_H__
#define __RECODER_THREAD_H__

#ifdef __cplusplus
extern "C" {
#endif

void rec_cmd_queue_init();
void set_rec_cmd_queue_p1(int p1);
int get_rec_cmd_queue_p1();
void set_rec_cmd_queue_p2(int p2);
int get_rec_cmd_queue_p2();
void set_rec_cmd_queue_state(int p, int state);
int get_rec_cmd_queue_state(int p);
void rec_start_init(int res, int time_lapse, int freq, int fpga_enc);
void recoder_thread_init();
void set_rec_state(int state) ;
int get_rec_state();
void set_rec_thread_en(int en);
int get_rec_thread_en();

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__RECODER_THREAD_H__