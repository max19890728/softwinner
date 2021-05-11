/* *******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 * *******************************************************************************/
/**
 * @file status_bar_window.h
 * @brief 主畫面控件管理器
 * @author UnpxreTW
 * @version v0.1
 * @date 2020-07-21
 */

#pragma once

#if 0

#include <signal.h>
#include <time.h>
#include <string>

#include "widgets/graphic_view.h"
#include "widgets/scroll_selector.h"
#include "widgets/text_view.h"
#include "window/preview_window.h"
#include "window/user_msg.h"
#include "window/window.h"
#include "window/window_manager.h"

class PreviewWindow;
class StatusBarMiddleWindow;
class StatusBarWindow : public SystemWindow {
  DECLARE_DYNCRT_CLASS(StatusBarWindow, Runtime)

 public:
  NotifyEvent OnClick;
  std::string GetResourceName();

  StatusBarWindow(IComponent *parent);

  virtual ~StatusBarWindow();

  void GetCreateParams(CommonCreateParams &params);

  void PreInitCtrl(View *ctrl, std::string &ctrl_name);

  int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);

  void Update(MSG_TYPE msg, int p_CamID = 0, int p_recordId = 0);
  void UpdatePhotoMode(int mode, int index);
  void SetWinStatus(int status);
  void OnLanguageChanged();
  void ReturnStatusBarBottomWindowHwnd();
  HWND GetSBBWHwnd();

  void UpdatePlaybackFileInfo(int index, int count, bool flag = true);
  void SetRecordStatusFlag(bool flag) { record_status_flag_ = flag; }
  void HidePlaybackBarIcon();
  static void PhotoingTimerProc(union sigval sigval);
  static void ChangemodetimerProc(union sigval sigval);
  void hideStatusBarWindow();
  void showStatusBarWindow();
  void StatusBarWindowProc(View *control);

  // for phototimer
  void Set_photocountdown_caption(char *text);

  bool m_changemode_enable;
  void SetStatusRecordStatus(int val) { RecordStatus = val; }

 private:
  HWND hwndel_;
  pthread_t m_battery_detect_thread_id;

  Uint32 m_bg_color;

  timer_t timer_id_;
  timer_t rechint_timer_id_;
  timer_t recording_timer_id_;
  timer_t changemode_timer_id_;

  GraphicView *background_test;  // 測試用透明漸層背景
  GraphicView *win_status_icon;  // 拍攝模式圖示
  ScrollSelector *scroll_test;

  StatusBarMiddleWindow *m_sbmiddle;

  TextView *_test_text_view;
  TextView *m_rec_file_time;

  int _testPointX = 120, _testPointY = 160;
  std::string _test_image_path;
  std::string _test_text_image_path;
  BITMAP _test_background_image;
  BITMAP _test_text_image;

  int win_status_;
  int RecordStatus;
  int m_current_battery_level;
  int m_current_4g_signal_level;
  bool m_time_update_4g;
  bool record_status_flag_;
  int m_RecordTime;
  int m_RecordTimeSave;

  int getModeConfigByIndex(int msg);

  void changeWindowStatusTo(WindowStatus newStatus);

  // MARK: - - - - - Record Timer - - - - -

 public:
  int currentRecordTime();

  void setRecordTimerToShow(bool toShow);

 private:
  void RecordTimeUiOnoff(int val);

  static void RecHintTimerProc(union sigval sigval);

  static void RecordingTimerProc(union sigval sigval);

  void initRecordTimer();

  void ResetRecordTime();

  // MARK: - - - - - Camera Mode - - - - -

  void WinstatusIconHander(int state);

  // MARK: - - - - - Shutter Delay - - - - -

  void setShutterDelayIconToShow(bool toShow);

  // MARK: - - - - - SD Card - - - - -

  void setSDCardIconToShow(bool toShow);

  // MARK: - - - - - Wifi - - - - -

  void setConnectionIconToShow(bool toShow);

  void setIsConnectToApp(bool isConnect);

  // MARK: - - - - - Battery - - - - -

  static void *BatteryDetectThread(void *context);

  void UpdateBatteryStatus(bool show);

  void UpdateBatteryStatus(bool sb, int levelval);
};

#endif
