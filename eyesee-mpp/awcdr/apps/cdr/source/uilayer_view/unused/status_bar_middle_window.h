/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file status_bar_window.h
 * @brief 状态栏窗口
 * @author id:826
 * @version v0.3
 * @date 2016-07-01
 */
#pragma once

#if 0

#include <signal.h>
#include <time.h>
#include "widgets/graphic_view.h"
#include "widgets/text_view.h"
#include "window/preview_window.h"
#include "window/user_msg.h"
#include "window/window.h"
#include "window/window_manager.h"

class PreviewWindow;

#define PHOTOTIMEUSEMIDDLE
class StatusBarMiddleWindow : public SystemWindow {
  DECLARE_DYNCRT_CLASS(StatusBarMiddleWindow, Runtime)

 private:
  HWND hwndel_;
  BITMAP background_image_;
  BITMAP scroll_background_image_;
  BITMAP options_unauto_image_;
  BITMAP options_auto_image_;
  BITMAP is_auto_image_;
  BITMAP is_unauto_image_;
  BITMAP select_frame_image_;
  BITMAP auto_switch_image_;

  bool is_auto;
  int win_status_;

  timer_t timer_id_data;
  timer_t scroll_inertia_timer_;

  TextView *time_label;

  int touch_point_y_;
  int last_point_y_;

 public:
  std::string GetResourceName();

  int scroll_offset_;
  int scroll_speed_;

  StatusBarMiddleWindow(IComponent *parent);

  virtual ~StatusBarMiddleWindow();

  void GetCreateParams(CommonCreateParams &params);

  void OnLanguageChanged();

  int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);

  void PreInitCtrl(View *ctrl, std::string &ctrl_name);

  void Update(MSG_TYPE msg, int p_CamID = 0, int p_recordId = 0);

  HWND GetSBBWHwnd();

  static void DateUpdateProc(union sigval sigval);

  static void ScrollInertiaProc(union sigval sigval);

  void setScrollInertiaStart();

  void setScrollInertiaEnd();

  void setScrollOffset(int offset);

  void GetIndexStringArray(std::string array_name, std::string &result,
                           int index);

  void GetString(std::string array_name, std::string &result);

  int GetModeConfigIndex(int msg);

  int GetStringArrayIndex(int msg);

  void SetWinStatus(int status);

  void ReturnStatusBarBottomWindowHwnd();

  void TimeStartStopCtrl(bool flag);

 private:
  void startScroll();

  void scrollEnd();
};

#endif
