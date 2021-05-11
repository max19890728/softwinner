/******************************************************************************
  Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 ******************************************************************************/
/**
 * @file main.h
 * @brief 该文件为应用程序入口，
 *
 *  定义程序入口及完成UI初始化及业务逻辑层各控制器的创建
 *
 * @author id:520
 * @date 2015-11-24
 *
 * @verbatim
    History:
    - 2016-06-01, id:826
      单独完成UI和业务逻辑层的初始化, 单独在一个线程中完成控制器的创建
      和初始化
   @endverbatim
 */

#pragma once

#include <minigui/common.h>
#include <minigui/minigui.h>

/**
 * @brief 应用程序入口
 *
 *  实际的入口为MiniGUIAppMain，这里定义宏是为了对使用者隐藏GUI的存在
 *
 * @param args 程序入口参数个数
 * @param argv 程序入口参数列表
 * @return 成功返回0, 出错返回-1
 */
#define Main                                                                  \
  MiniGUIAppMain(int args, const char *argv[]);                               \
  int main_entry(int args, const char *argv[]) {                              \
    int ret = 0;                                                              \
    struct timeval tv;                                                        \
    gettimeofday(&tv, NULL);                                                  \
    fprintf(stderr, "%s time: %lds.%ldms\n", "start minigui init", tv.tv_sec, \
            tv.tv_usec);                                                      \
    setenv("FB_SYNC", "1", 1);                                                \
    setenv("SCREEN_INFO", SCREEN_INFO, 1);                                    \
    gettimeofday(&tv, NULL);                                                  \
    fprintf(stderr, "%s time: %lds.%ldms\n", "end minigui init", tv.tv_sec,   \
            tv.tv_usec);                                                      \
    ret = MiniGUIAppMain(args, argv);                                         \
    TerminateGUI(ret);                                                        \
    return ret;                                                               \
  }                                                                           \
  int MiniGUIAppMain
