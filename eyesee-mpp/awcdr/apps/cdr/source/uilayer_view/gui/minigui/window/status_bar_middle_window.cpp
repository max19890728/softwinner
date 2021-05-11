/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file status_bar_window.cpp
 * @brief 状态栏窗口
 * @author id:826
 * @version v0.3
 * @date 2016-07-01
 */

#include "window/status_bar_middle_window.h"

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "bll_presenter/audioCtrl.h"
#include "common/app_def.h"
#include "common/message.h"
#include "common/posix_timer.h"
#include "common/setting_menu_id.h"
#include "debug/app_log.h"
#include "device_model/media/media_definition.h"
#include "device_model/menu_config_lua.h"
#include "device_model/storage_manager.h"
#include "device_model/system/event_manager.h"
#include "device_model/system/power_manager.h"
#include "resource/resource_manager.h"
#include "widgets/card_view.h"
#include "widgets/view_container.h"
#include "window/window.h"
#include "window/playback_window.h"
#include "window/window_manager.h"

#undef LOG_TAG
#define LOG_TAG "MiddleWindow"

IMPLEMENT_DYNCRT_CLASS(StatusBarMiddleWindow)

std::string StatusBarMiddleWindow::GetResourceName() {
  return std::string(GetClassName());
}

StatusBarMiddleWindow::StatusBarMiddleWindow(IComponent *parent)
    : SystemWindow(parent), win_status_(STATU_PREVIEW), hwndel_(HWND_INVALID) {
  wname = "StatusBarMiddleWindow";
  Load();
  R::get()->SetLangID(GetModeConfigIndex(SETTING_DEVICE_LANGUAGE));
  SetBackColor(0xFF777777);
  ::SetWindowFont(hwndel_, R::get()->GetFontBySize(34));
  // SetWindowBackImage(R::get()->GetImagePath("test_bg").c_str());
  std::string background_image_path_ = R::get()->GetImagePath("test_bg");
  // std::string background_image_path_ = R::get()->GetImagePath("test_black_bg");
  db_debug("Path: %s", background_image_path_.c_str());
  // SetWindowBackImage(background_image_path_.c_str());
  LoadBitmapFromFile(HDC_SCREEN, &background_image_, background_image_path_.c_str());

  LoadBitmapFromFile(HDC_SCREEN, &scroll_background_image_, R::get()->GetImagePath("test02").c_str());
  LoadBitmapFromFile(HDC_SCREEN, &options_auto_image_, R::get()->GetImagePath("test01").c_str());
  LoadBitmapFromFile(HDC_SCREEN, &options_unauto_image_, R::get()->GetImagePath("test05").c_str());
  LoadBitmapFromFile(HDC_SCREEN, &is_auto_image_, R::get()->GetImagePath("test04").c_str());
  LoadBitmapFromFile(HDC_SCREEN, &is_unauto_image_, R::get()->GetImagePath("test03").c_str());
  // LoadBitmapFromFile(HDC_SCREEN, &select_frame_image_, R::get()->GetImagePath("test07").c_str());
  // LoadBitmapFromFile(HDC_SCREEN, &auto_switch_image_, R::get()->GetImagePath("test00").c_str());
  is_auto = true;

#if 0
  GraphicView::LoadImage(GetControl("background_test"), "test_bg");
  GetControl("background_test")->Show();
#else
  // GetControl("background_test")->Hide();
#endif

  // time_label = reinterpret_cast<TextView *>(GetControl("time_label"));
  // SetWindowFont(time_label->GetHandle(), R::get()->GetFontBySize(34));
  // time_label->SetTimeCaption("Size 測試");
  // time_label->SetCaptionColor(0xFFFFFFFF);
  // 改变字体背景色后需要同步修改字体控件SetBrushColor(hdc, 0x00000000);
  // time_label->SetBackColor(0x00000000);
  // SetBrushColor(time_label->GetHandle(), 0x00000000);

  ReturnStatusBarBottomWindowHwnd();

  scroll_speed_ = 0;
  create_timer(this, &scroll_inertia_timer_, ScrollInertiaProc);
  stop_timer(scroll_inertia_timer_);
  //set_period_timer(0, 64000000, scroll_inertia_timer_);

  // create_timer(this, &timer_id_data, DateUpdateProc);
  // stop_timer(timer_id_data);
  // set_period_timer(0, 320000000, timer_id_data);

  scroll_offset_ = 0;
}

StatusBarMiddleWindow::~StatusBarMiddleWindow() {
  db_msg("destruct");
  ::delete_timer(timer_id_data);
}

void StatusBarMiddleWindow::GetCreateParams(CommonCreateParams &params) {
  params.class_name = " ";
  params.alias = GetClassName();
  params.style = WS_NONE | SS_REALSIZEIMAGE | WS_VISIBLE | SS_NOTIFY | SS_BITMAP;
  params.exstyle = WS_EX_NONE | WS_EX_TOPMOST | WS_EX_TRANSPARENT;
}

void StatusBarMiddleWindow::OnLanguageChanged() {}

int StatusBarMiddleWindow::HandleMessage(HWND hwnd, int message, WPARAM wparam,
                                         LPARAM lparam) {
  switch (message) {
#if 0
    case MSG_ERASEBKGND: {
      int res = ContainerWidget::HandleMessage(hwnd, message, wparam, lparam);
      db_debug("MSG_ERASEBKGND");
      HDC _hdc = BeginPaint(hwnd);
      FillBoxWithBitmap(_hdc, 0, 0, 240, 320, &background_image_);
      EndPaint(hwnd, _hdc);
      return DO_IT_MYSELF;
    }
#endif
    case MSG_PAINT: {
      db_debug("MSG_PAINT");
      HDC _hdc = BeginPaint(hwnd);
      HDC _second = CreateCompatibleDC(_hdc);
      BitBlt(_hdc, 0, 0, 240, 320, _second, 0, 0, 0);
      FillBoxWithBitmapPart(
        _second,
        240 - scroll_background_image_.bmWidth, 0,
        scroll_background_image_.bmWidth, 320,
        0, 0,
        &scroll_background_image_,
        0, scroll_offset_);
      if (is_auto) {
        FillBoxWithBitmapPart(
          _second,
          240 - options_auto_image_.bmWidth, 0,
          options_auto_image_.bmWidth, 320,
          0, 0,
          &options_auto_image_,
          0, scroll_offset_);
        FillBoxWithBitmap(
          _second,
          0, (320 - is_auto_image_.bmHeight) / 2,
          0, 0,
          &is_auto_image_);
      } else {
        FillBoxWithBitmapPart(
          _second,
          240 - options_unauto_image_.bmWidth, 0,
          options_unauto_image_.bmWidth, 320,
          0, 0,
          &options_unauto_image_,
          0, scroll_offset_);
        FillBoxWithBitmap(
          _second,
          0, (320 - is_unauto_image_.bmHeight) / 2,
          0, 0,
          &is_unauto_image_);
      }
      // FillBoxWithBitmap(
      //   _second,
      //   0, (320 - select_frame_image_.bmHeight) / 2,
      //   0, 0,
      //   &select_frame_image_);
      // int res = FillBoxWithBitmap(_second, 0, 0, 240, 320, &background_image_);
      // db_debug("%s, %d", res == TRUE ? "TRUE" : "FLASE", background_image_.bmType);
      BitBlt(_second, 0, 0, 240, 320, _hdc, 0, 0, 0);
      DeleteCompatibleDC(_second);
      EndPaint(hwnd, _hdc);
      return HELP_ME_OUT;
    }
    case MSG_LBUTTONDOWN:
      db_debug("MSG_LBUTTONDOWN");
      setScrollInertiaEnd();
      touch_point_y_ = HIWORD(lparam);
      scroll_speed_ = 0;
      return HELP_ME_OUT;
    case MSG_LBUTTONUP:
      db_debug("MSG_LBUTTONUP");
      setScrollInertiaStart();
      touch_point_y_ = HIWORD(lparam);
      return HELP_ME_OUT;
    case MSG_MOUSEMOVE: {
      // db_debug("MSG_MOUSEMOVE");
      scroll_speed_ = touch_point_y_ - HIWORD(lparam);
      scroll_offset_ += scroll_speed_;
      if (scroll_offset_ < 0) scroll_offset_ = 0;
      if (scroll_offset_ > scroll_background_image_.bmHeight - 320)
        scroll_offset_ = scroll_background_image_.bmHeight - 320;
      touch_point_y_ = HIWORD(lparam);
      Refresh();
      return HELP_ME_OUT;
    }
    default:
      return ContainerWidget::HandleMessage(hwnd, message, wparam, lparam);
  }
}

void StatusBarMiddleWindow::PreInitCtrl(View *ctrl, std::string &ctrl_name) {
  if (ctrl_name == "time_label") {
    ctrl->SetCtrlTransparentStyle(true);
    TextView *time_label_ = reinterpret_cast<TextView *>(ctrl);
    time_label_->SetTextStyle(DT_VCENTER | DT_CENTER);
  } else {
    ctrl->SetCtrlTransparentStyle(true);
  }
}

void StatusBarMiddleWindow::Update(MSG_TYPE msg, int p_CamID, int p_recordId) {
  db_msg("handle msg: %d, win_status_ = %d", msg, win_status_);
  WindowManager *win_mg = ::WindowManager::GetInstance();
  if (win_mg->GetisWindowChanging()) {
    db_warn("window is now change not respone cmd");
    return;
  }
  switch (msg) {
    case MSG_SETTING_TO_PREIVEW_CHANG_STATUS_BAR_BOTTOM: {
      db_msg("zhb---MSG_PLAYBACK_TO_PREIVEW_CHANG_STATUS_BAR_BOTTOM");
      WindowManager *win_mg = ::WindowManager::GetInstance();
      PreviewWindow *pre_win =
          static_cast<PreviewWindow *>(win_mg->GetWindow(WINDOWID_PREVIEW));
      int win_statu_save = pre_win->Get_win_statu_save();
      db_error("win_statu_save: %d", win_statu_save);
      if (win_statu_save == STATU_PREVIEW) {
        // SetStatusPreview();
      } else if (win_statu_save == STATU_PHOTO) {
        // SetStatusPhoto();
      }
    } break;
    case MSG_PLAYBACK_TO_PLAY_WINDOW:
      win_status_ = STATU_PLAYBACK;
      break;
    default:
      break;
  }
}

HWND StatusBarMiddleWindow::GetSBBWHwnd() { return hwndel_; }

void StatusBarMiddleWindow::ReturnStatusBarBottomWindowHwnd() {
  hwndel_ = GetHandle();
}

void StatusBarMiddleWindow::DateUpdateProc(union sigval sigval) {
  char buf[32] = {0};
  char buf2[32] = {0};
  struct tm *tm = NULL;
  time_t timer;
  prctl(PR_SET_NAME, "UpdateDateTime", 0, 0, 0);
  db_debug("Update");
  timer = time(NULL);
  tm = localtime(&timer);
  // tm = localtime_r(&timer, tm);

  snprintf(buf, sizeof(buf), "%04d-%02d-%02d  %02d:%02d:%02d",
           tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour,
           tm->tm_min, tm->tm_sec);

  StatusBarMiddleWindow *sb =
      reinterpret_cast<StatusBarMiddleWindow *>(sigval.sival_ptr);

  if ((sb->win_status_ == STATU_PREVIEW) || (sb->win_status_ == STATU_PHOTO)) {
    sb->time_label->SetCaption(buf);
  } else {
    sb->time_label->SetTimeCaption("");
  }
}

void StatusBarMiddleWindow::ScrollInertiaProc(union sigval sigval) {
  StatusBarMiddleWindow *statusBar = reinterpret_cast<StatusBarMiddleWindow *>(sigval.sival_ptr);
  int offset = statusBar->scroll_offset_;
  int speed = statusBar->scroll_speed_;
  db_debug("speed: %d", speed);
  if (speed < 10 && speed > -10) {
    statusBar->setScrollInertiaEnd();
  }
  speed *= 0.98;
  statusBar->scroll_offset_ += speed;
  statusBar->scroll_speed_ = speed;
  if (statusBar->scroll_offset_ < 0) statusBar->scroll_offset_ = 0;
  if (statusBar->scroll_offset_ > statusBar->scroll_background_image_.bmHeight - 320)
    statusBar->scroll_offset_ = statusBar->scroll_background_image_.bmHeight - 320;
  statusBar->Refresh();
}

void StatusBarMiddleWindow::setScrollInertiaStart() {
  set_period_timer(0, 160000000, scroll_inertia_timer_);
}

void StatusBarMiddleWindow::setScrollInertiaEnd() {
  stop_timer(scroll_inertia_timer_);
}

void StatusBarMiddleWindow::setScrollOffset(int offset) {

}

int StatusBarMiddleWindow::GetModeConfigIndex(int msg) {
  int index = -1;
  MenuConfigLua *mfl = MenuConfigLua::GetInstance();
  index = mfl->GetMenuIndexConfig(msg);
  return index;
}

void StatusBarMiddleWindow::TimeStartStopCtrl(bool flag) {
  if (!flag) {
    TextView *time_label =
        reinterpret_cast<TextView *>(GetControl("time_label"));
    stop_timer(timer_id_data);
    time_label->SetTimeCaption(" ");
  } else {
    TextView *time_label =
        reinterpret_cast<TextView *>(GetControl("time_label"));
    set_period_timer(1, 0, timer_id_data);
  }
}

void StatusBarMiddleWindow::GetIndexStringArray(std::string array_name,
                                                std::string &result,
                                                int index) {
  StringVector title_str1;
  std::vector<std::string>::const_iterator it;
  R::get()->GetStringArray(array_name, title_str1);
  it = title_str1.begin() + index;
  result = (*it).c_str();
}

void StatusBarMiddleWindow::GetString(std::string array_name,
                                      std::string &result) {
  std::string title_str1;
  R::get()->GetString(array_name, title_str1);
  result = title_str1;
}

int StatusBarMiddleWindow::GetStringArrayIndex(int msg) {
  int index = -1;
  MenuConfigLua *mfl = MenuConfigLua::GetInstance();
  index = mfl->GetMenuIndexConfig(msg);
  return index;
}

void StatusBarMiddleWindow::SetWinStatus(int status) { win_status_ = status; }
