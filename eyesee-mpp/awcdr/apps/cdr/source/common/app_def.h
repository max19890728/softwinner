#pragma once

#include "common/app_platform.h"

#define EXIT_APP 0
#define POWEROFF 1
#define REBOOT 2

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define SCREEN_INFO "320x240-32bpp"

#define MODEL_PREFIX "V536-CDR"  // "V536-CDR"

#define USE_CAMA

#define USE_CAMB
#define CAMB_PREVIEW

// #define USE_IMX335  // 使用imx335打开宏定义
#define LONGPRESSTIME_FOR_PWR 250  // unit: 10ms

// #define SUPPORT_AUTOHIDE_STATUSBOTTOMBAR

#define USEICONTHUMB
#define PHOTO_MODE

#define LCD_BRIGHTNESSLEVEL 1  // 0 最亮 ~ 6 最暗
// #define SUPPORT_RECTIMELAPS  // 缩时录影
#define VIDEOTYPE_MP4  // if def = mp4 otherwise ts
#define SUPPORT_CHANGEPARKFILE

// #define MEM_DEBUG

// #define SUPPORT_TFCARDIN_RECORD  // 插卡自动录像
#define INFO_ZD55

#define RTCASUTC  // 使用rtc时间为utc时间
// #define TF653_KEY       // 使用tf653按键定义

// #define GSENSOR_SUSPEND_ENABLE // 允许GSENSOR 进入挂起模式(0.7uA最省电)

#define PROMPT_POWEROFF_TIME 5  // 5
// #define SUPPORT_MODEBUTTON_TOP
// 模式切换键在左上角(公版),否则在底部状态栏(致君)

// #define SUPPOTR_SHOWSPEEDUI  // 在预览界面显示当前速度
// #define SUPPORT_RECORDTYPEPART  // 录像文件分区

#define STOPCAMERA_TO_PLAYBACK  // 进入playback关闭camera

// #define SUPPORT_PSKIP_ENABLE  // 打开 允许插空帧, 关闭 不插空帧

#define RECORDER_FIXSIZE  // 固定录像文件尺寸

#define SHOW_DEBUG_VIEW true
#define AUDIO_ENABLE false

#define USE_REAL_LAYER false
