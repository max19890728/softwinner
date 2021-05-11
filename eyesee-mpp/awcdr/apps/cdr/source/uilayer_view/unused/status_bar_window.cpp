/* *******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 * *******************************************************************************/
/**
 * @file status_bar_window.cpp
 * @brief 主畫面控件管理器
 * @author UnpxreTW
 * @version v0.1
 * @date 2020-07-21
 */

#if 0

#include "window/status_bar_window.h"
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include "bll_presenter/device_setting.h"
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

#undef LOG_TAG
#define LOG_TAG "StatusBarWindow"

#define BATTERY_DETECT_TIME 1

IMPLEMENT_DYNCRT_CLASS(StatusBarWindow)

std::string StatusBarWindow::GetResourceName() {
  return std::string(GetClassName());
}

StatusBarWindow::StatusBarWindow(IComponent *parent)
    : SystemWindow(parent),
      win_status_(STATU_PREVIEW),
      m_current_battery_level(-1),
      hwndel_(HWND_INVALID),
      record_status_flag_(false),
      m_RecordTime(0),
      m_bg_color(0x00000000),
      m_changemode_enable(false) {
  wname = "StatusBarWindow";
  db_error("StatusBarWindow");
  Load();
  R::get()->SetLangID(getModeConfigByIndex(SETTING_DEVICE_LANGUAGE));
  SetBackColor(m_bg_color);
  ReturnStatusBarBottomWindowHwnd();
  RecordStatus = 0;
  initRecordTimer();

  ::SetWindowFont(GetHandle(), R::get()->GetFontBySize(200));

#if 1
  background_test =
      reinterpret_cast<GraphicView *>(GetControl("background_test"));
  GraphicView::LoadImage(GetControl("background_test"), "test_bg");
  background_test->Hide();
#endif

  win_status_icon = reinterpret_cast<GraphicView *>(GetControl("status_icon"));
  GraphicView::LoadImage(GetControl("status_icon"), "preview");

  TextView *textView = reinterpret_cast<TextView *>(GetControl("testBigText"));
  textView->Hide();
  textView->SetText("0");
  textView->SetCaptionColor(0xFFFFFFFF);
  _test_text_view = textView;

#ifdef PHOTO_MODE
  // win_status_icon->Show();
  win_status_icon->Hide();
#ifdef SUPPORT_MODEBUTTON_TOP
  win_status_icon->OnClick.bind(this, &StatusBarWindow::StatusBarWindowProc);
#endif
#else
  win_status_icon->Hide();
#endif

  setConnectionIconToShow(false);
  setSDCardIconToShow(false);
  setShutterDelayIconToShow(false);

  ThreadCreate(&m_battery_detect_thread_id, NULL,
               StatusBarWindow::BatteryDetectThread, this);

  setRecordTimerToShow(false);
  RecordTimeUiOnoff(0);
  WindowManager *win_mg_ = WindowManager::GetInstance();
  m_sbmiddle = static_cast<StatusBarMiddleWindow *>(
      win_mg_->GetWindow(WINDOWID_STATUSBAR_MIDDLE));
  m_sbmiddle->Show();
}

StatusBarWindow::~StatusBarWindow() {
  db_msg("destruct");
  if (m_battery_detect_thread_id > 0)
    pthread_cancel(m_battery_detect_thread_id);
  ::stop_timer(rechint_timer_id_);
  ::delete_timer(rechint_timer_id_);
  ::stop_timer(recording_timer_id_);
  ::delete_timer(recording_timer_id_);
}

void StatusBarWindow::GetCreateParams(CommonCreateParams &params) {
  params.style = WS_NONE;
  params.exstyle = WS_EX_NONE | WS_EX_TOPMOST | WS_EX_TRANSPARENT;
  params.class_name = " ";
  params.alias = GetClassName();
}

void StatusBarWindow::PreInitCtrl(View *ctrl, std::string &ctrl_name) {
  if (ctrl_name == "rec_time_label") {
    ctrl->SetCtrlTransparentStyle(false);
    TextView *time_label_ = reinterpret_cast<TextView *>(ctrl);
    time_label_->SetTextStyle(DT_VCENTER | DT_CENTER);
  } else {
    ctrl->SetCtrlTransparentStyle(true);
  }
}

void StatusBarWindow::ReturnStatusBarBottomWindowHwnd() {
  hwndel_ = GetHandle();
}

HWND StatusBarWindow::GetSBBWHwnd() { return hwndel_; }

int StatusBarWindow::HandleMessage(HWND hwnd, int message, WPARAM wparam,
                                   LPARAM lparam) {
  switch (message) {
    case MSG_PAINT: {
      db_debug("MSG_PAINT");
      HDC __hdc = BeginPaint(hwnd);
      HDC __sec_dc = CreateCompatibleDC(__hdc);
      SetTextColor(__sec_dc, 0xFF0000FF);
      SetBkMode(__sec_dc, BM_TRANSPARENT);
      int __originX = _testPointX - 120, __originY = _testPointY - 160;
      RECT text_rect = {__originX, __originY, __originX + 240, __originY + 320};
      double __scale = 0.5 + (static_cast<double>(_testPointY) / 320) * 0.5;
      BitBlt(__hdc, 0, 0, 240, 320, __sec_dc, 0, 0, 0);
      FillBoxWithBitmapPart(
        __sec_dc,
        (120 - (120 * __scale)), __originY + 0,
        240 * __scale, 160 * __scale,
        240 * __scale, 320 * __scale,
        &_test_background_image,
        0, 60 * __scale);
      FillBoxWithBitmapPart(
        __sec_dc,
        0, __originY + 160, 240, 100,
        0, 0,
        &_test_background_image,
        0, 220);
      FillBoxWithBitmapPart(
        __sec_dc,
        0, __originY + 260, 240, 60,
        0, 0,
        &_test_background_image,
        0, 0);
      DrawText(__sec_dc, "測", -1, &text_rect,
               DT_CENTER | DT_VCENTER | DT_SINGLELINE);
      FillBoxWithBitmap(__sec_dc, __originX + 70, __originY + 240, 100, 60,
                        &_test_text_image);
      BitBlt(__sec_dc, 0, 0, 240, 320, __hdc, 0, 0, 0);
      DeleteCompatibleDC(__sec_dc);
      EndPaint(hwnd, __hdc);
      return HELP_ME_OUT;
    }
    case MSG_LBUTTONDOWN:
      db_debug("touchdown, x: %d, y: %d", LOWORD(lparam), HIWORD(lparam));
      break;
    case MSG_MOUSEMOVE:
      _testPointX = LOWORD(lparam);
      _testPointY = HIWORD(lparam);
      db_debug("move, x: %d y: %d", _testPointX, _testPointY);
      Refresh();
      return HELP_ME_OUT;
    default:
      return ContainerWidget::HandleMessage(hwnd, message, wparam, lparam);
  }
}

void StatusBarWindow::Update(MSG_TYPE msg, int p_CamID, int p_recordId) {
  db_debug("handle msg: %d, win_status_: %d", msg, win_status_);
  WindowManager *win_mg = ::WindowManager::GetInstance();
  switch (msg) {
    case MSG_WIFI_DISABLED:
    case MSG_SOFTAP_DISABLED:
    case MSG_SOFTAP_ENABLE:
    case MSG_SOFTAP_ENABLED:
    case MSG_WIFI_ENABLE:
    case MSG_WIFI_ENABLED:
    case MSG_WIFI_DISCONNECTED:
    case MSG_WIFI_CONNECTED:
      setConnectionIconToShow(true);
      break;
    case MSG_STORAGE_MOUNTED:
      db_error("MSG_STORAGE_MOUNTED");
      setSDCardIconToShow(true);
      setRecordTimerToShow(false);
      break;
    case MSG_STORAGE_UMOUNT:
      db_error("MSG_STORAGE_UNMOUNT");
      setSDCardIconToShow(true);
      setRecordTimerToShow(false);
      break;
    case MSG_SETTING_TO_PREIVEW_CHANG_STATUS_BAR_BOTTOM:
    case MSG_PLAYBACK_TO_PREIVEW_CHANG_STATUS_BAR_BOTTOM: {
      db_msg("zhb---MSG_PLAYBACK_TO_PREIVEW_CHANG_STATUS_BAR_BOTTOM");
      PreviewWindow *pre_win =
          static_cast<PreviewWindow *>(win_mg->GetWindow(WINDOWID_PREVIEW));
      int win_statu_save = pre_win->Get_win_statu_save();
      db_error(
          "zhb---MSG_SETTING_TO_PREIVEW_CHANG_STATUS_BAR_BOTTOM "
          "win_statu_save: %d",
          win_statu_save);
      if (win_statu_save == STATU_PREVIEW) {
        changeWindowStatusTo(STATU_PREVIEW);
      } else if (win_statu_save == STATU_PHOTO) {
        // SetStatusPhoto();
      }
      break;
    }
    case MSG_CHANG_STATU_PREVIEW:
      changeWindowStatusTo(STATU_PREVIEW);
      break;
    case MSG_PLAY_TO_PLAYBACK_WINDOW:
    case MSG_CHANG_STATU_PLAYBACK:
      changeWindowStatusTo(STATU_PLAYBACK);
      break;
    case MSG_PREVIW_TO_SETTING_CHANGE_STATUS_BAR:
      changeWindowStatusTo(STATU_SETTING);
      break;
    case MSG_PLAYBACK_TO_PLAY_WINDOW:
      db_msg("[debug_zhb]----MSG_PLAYBACK_TO_PLAY_WINDOW");
      win_status_ = STATU_PLAYBACK;
      break;
    case MSG_APP_IS_CONNECTED:
      db_warn("msg is MSG_APP_IS_CONNECTED should update the icon\n");
      setConnectionIconToShow(false);
      setIsConnectToApp(true);
      break;
    case MSG_APP_IS_DISCONNECTED:
      db_warn("msg is MSG_APP_IS_DISCONNECTED should update the icon\n");
      setIsConnectToApp(false);
      setConnectionIconToShow(true);
      break;
    default:
      break;
  }
}

void StatusBarWindow::ChangemodetimerProc(union sigval sigval) {
  prctl(PR_SET_NAME, "Changemodetime", 0, 0, 0);
  StatusBarWindow *sbw = reinterpret_cast<StatusBarWindow *>(sigval.sival_ptr);
  sbw->m_changemode_enable = true;
  ::delete_timer(sbw->changemode_timer_id_);
  db_error("m_changemode_enable set True");
}

void StatusBarWindow::UpdatePlaybackFileInfo(int index, int count, bool flag) {
  return;
}

void StatusBarWindow::HidePlaybackBarIcon() {
  if (record_status_flag_ == true) {
    if (!GetControl("rec_hint_icon")->GetVisible())
      GetControl("rec_hint_icon")->Show();
    set_period_timer(1, 0, rechint_timer_id_);
  } else {
    stop_timer(rechint_timer_id_);
  }
}

void StatusBarWindow::OnLanguageChanged() {}

void StatusBarWindow::SetWinStatus(int status) { win_status_ = status; }

void StatusBarWindow::hideStatusBarWindow() {
  // if (this->m_sbmiddle->GetVisible())
  // this->m_sbmiddle->Hide();
  // this->m_sbmiddle->DateTimeUiOnoff(0);
  if (this->GetVisible()) this->Hide();
}

void StatusBarWindow::showStatusBarWindow() {
  if (/*!this->m_sbmiddle->GetVisible()*/ 1) {
    // this->m_sbmiddle->Show();
    // this->m_sbmiddle->DateTimeUiOnoff(1);
  }
  if (!this->GetVisible()) this->Show();
}

void StatusBarWindow::StatusBarWindowProc(View *control) {
  db_info("Testing");
#ifdef PHOTO_MODE
#ifdef SUPPORT_MODEBUTTON_TOP
  WindowManager *win_mg = ::WindowManager::GetInstance();
  // if (m_changemode_enable) { // 允许切换模式?
  if (win_mg->GetCurrentWinID() == WINDOWID_PREVIEW) {
    PreviewWindow *pre_win =
        static_cast<PreviewWindow *>(win_mg->GetWindow(WINDOWID_PREVIEW));
    int curstatus = pre_win->GetWindowStatus();
    db_error("========click button change window status(old): %d", curstatus);
    if (curstatus == STATU_PREVIEW) {
      curstatus = STATU_PHOTO;
    } else {
      curstatus = STATU_PREVIEW;
    }
    ::SendMessage(pre_win->GetHandle(), MSG_CHANGEWINDOWMODE, curstatus, 0);
  }
//}
#endif
#endif
}

int StatusBarWindow::getModeConfigByIndex(int msg) {
  return MenuConfigLua::GetInstance()->GetMenuIndexConfig(msg);
}

void StatusBarWindow::changeWindowStatusTo(WindowStatus newStatus) {
  db_debug("Change WindowStatus from %d to %d", win_status_, newStatus);
  win_status_ = newStatus;
  switch (newStatus) {
    case STATU_SETTING:
    case STATU_PLAYBACK:
      if (this->is_visible_) this->SetVisible(false);
      break;
    case STATU_PREVIEW:
      if (this->is_visible_) this->SetVisible(true);
      break;
    default:
      break;
  }
}

// MARK: - - - - - Record Timer - - - - -

int StatusBarWindow::currentRecordTime() { return m_RecordTime; }

void StatusBarWindow::setRecordTimerToShow(bool toShow) {
  db_debug("Set Record Timer to %s", toShow ? "Show" : "Hide");
  View *icon_ = GetControl("rec_hint_icon");
  if (toShow) {
    m_RecordTime = 0;
    if (!icon_->GetVisible()) icon_->Show();
    set_period_timer(1, 0, rechint_timer_id_);
    if (!m_rec_file_time->GetVisible()) m_rec_file_time->Show();
    set_period_timer(1, 0, recording_timer_id_);
  } else {
    icon_->Hide();
    m_rec_file_time->SetTimeCaption("");
    m_rec_file_time->Hide();
    stop_timer(rechint_timer_id_);
    stop_timer(recording_timer_id_);
    system("echo 0 > /sys/class/leds/rec_led/brightness");
  }
}

void StatusBarWindow::RecordTimeUiOnoff(int val) {
  if (!val) {
    m_rec_file_time->Hide();
  } else {
    m_rec_file_time->Show();
  }
  db_warn("RecordTimeUiOnoff: %d", val);
}

void StatusBarWindow::RecHintTimerProc(union sigval sigval) {
  prctl(PR_SET_NAME, "UpdateRecHint", 0, 0, 0);
  static bool flag = false;
  StatusBarWindow *self = reinterpret_cast<StatusBarWindow *>(sigval.sival_ptr);
  if (self->RecordStatus) {
    if (flag) {
      self->GetControl("rec_hint_icon")->Hide();
      system("echo 0 > /sys/class/leds/rec_led/brightness");
    } else {
      self->GetControl("rec_hint_icon")->Show();
      system("echo 1 > /sys/class/leds/rec_led/brightness");
    }
    flag = !flag;
  } else {
    self->GetControl("rec_hint_icon")->Hide();
    system("echo 0 > /sys/class/leds/rec_led/brightness");
  }
}

void StatusBarWindow::RecordingTimerProc(union sigval sigval) {
  prctl(PR_SET_NAME, "UpdateRecordTime", 0, 0, 0);
  char buf[32] = {0};
  int rec_time = 0;
  StatusBarWindow *self = reinterpret_cast<StatusBarWindow *>(sigval.sival_ptr);
  if (self->RecordStatus) {
    rec_time = self->m_RecordTime++;
  } else {
    rec_time = self->m_RecordTimeSave;
  }
  snprintf(buf, sizeof(buf), "%02d:%02d", rec_time / 60 % 60, rec_time % 60);
  self->m_rec_file_time->SetTimeCaption(buf);
}

void StatusBarWindow::initRecordTimer() {
  View *icon_ = GetControl("rec_hint_icon");
  TextView *label_ = reinterpret_cast<TextView *>(GetControl("rec_time_label"));
  GraphicView::LoadImage(icon_, "rec_hint");
  if (icon_->GetVisible()) icon_->Hide();
  label_->SetCaptionColor(0xFFFFFFFF);
  label_->SetBackColor(m_bg_color);
  label_->SetTimeCaption("00:00");
  label_->Hide();
  m_rec_file_time = label_;
  create_timer(this, &recording_timer_id_, RecordingTimerProc);
  stop_timer(recording_timer_id_);
  create_timer(this, &rechint_timer_id_, RecHintTimerProc);
  stop_timer(rechint_timer_id_);
}

void StatusBarWindow::ResetRecordTime() {
  db_warn("mRecordTime: %d, need reset to 0", m_RecordTime);
  m_RecordTimeSave = m_RecordTime;
  m_RecordTime = 0;
}

// MARK: - - - - - Camera Mode - - - - -

void StatusBarWindow::WinstatusIconHander(int state) {
  db_debug("Set Camera Mode To %d", state);
  if (state < 0) {
    win_status_icon->Hide();
  } else {
    win_status_icon->Hide();
    switch (state) {
      case STATU_PREVIEW:
        GraphicView::LoadImage(win_status_icon, "preview");
        break;
      case STATU_PHOTO:
        GraphicView::LoadImage(win_status_icon, "photo");
        break;
      case STATU_SLOWRECOD:
      // GraphicView::LoadImage(win_status_icon, "slowrec");
      // break;
      case STATU_PLAYBACK:
      // GraphicView::LoadImage(win_status_icon, "playback");
      // break;
      case STATU_SETTING:
      case STATU_BINDING:
      case STATU_DELAYRECIRD:
      default:
        break;
    }
    win_status_icon->Show();
  }
}

// MARK: - - - - - Shutter Delay - - - - -

void StatusBarWindow::setShutterDelayIconToShow(bool toShow) {
  View *icon_ = GetControl("shutter_delay_icon");
  if (toShow) {
    switch (getModeConfigByIndex(SETTING_RECORD_LOOP)) {
      case 0:
        GraphicView::LoadImage(icon_, "s_top_looprec_1");
        break;
      case 1:
        GraphicView::LoadImage(icon_, "s_top_looprec_2");
        break;
      case 2:
        GraphicView::LoadImage(icon_, "s_top_looprec_5");
        break;
      default:
        break;
    }
    icon_->Show();
  } else {
    icon_->Hide();
  }
}

// MARK: - - - - - SD Card - - - - -

void StatusBarWindow::setSDCardIconToShow(bool toShow) {
  return;
  db_debug("Set SD Card Icon To %s", toShow ? "Show" : "Hide");
  View *icon_ = GetControl("sdcard_icon");
  if (toShow) {
    StorageManager *storageManager = StorageManager::GetInstance();
    if (storageManager->GetStorageStatus() == UMOUNT)
      GraphicView::LoadImage(icon_, "s_top_sdcard_off");
    else
      GraphicView::LoadImage(icon_, "s_top_sdcard_on");
    icon_->Show();
  } else {
    icon_->Hide();
  }
}

// MARK: - - - - - Wifi - - - - -

void StatusBarWindow::setConnectionIconToShow(bool toShow) {
  return;
  db_debug("Set Connection Icon To %s", toShow ? "Show" : "Hide");
  View *icon_ = GetControl("wifi_icon");
  if (toShow) {
    if (getModeConfigByIndex(SETTING_WIFI_SWITCH) == 1)
      GraphicView::LoadImage(icon_, "s_top_wifi_on");
    else
      GraphicView::LoadImage(icon_, "s_top_wifi_off");
    icon_->Show();
  } else {
    icon_->Hide();
  }
}

void StatusBarWindow::setIsConnectToApp(bool isConnect) {
  db_debug("Set is %s", isConnect ? "Connect to app" : "Disconnect to app");
  View *icon_ = GetControl("wifi_icon");
  if (isConnect) {
    GraphicView::LoadImage(icon_, "s_top_app_connect");
    icon_->Show();
  } else {
    icon_->Hide();
  }
}

// MARK: - - - - - Battery - - - - -

void *StatusBarWindow::BatteryDetectThread(void *context) {
  int battery_cap_level = -1;
  int batteryStatus;
  StatusBarWindow *p_statusBar = reinterpret_cast<StatusBarWindow *>(context);
  PowerManager *pm = PowerManager::GetInstance();
  WindowManager *win_mg = ::WindowManager::GetInstance();
  while (1) {
    if (pm->getACconnectStatus() == 1 &&
        !PowerManager::GetInstance()->getUsbconnectStatus()) {
      battery_cap_level = pm->GetBatteryLevel();
      // db_error("zhb---111battery_cap_level = %d",battery_cap_level);
      if (battery_cap_level != 6) {
        battery_cap_level = 4;  // charging
      } else {
        // battery_cap_level = 7; // charge full
      }
    } else if (pm->getACconnectStatus() == 1 &&
               PowerManager::GetInstance()->getUsbconnectStatus()) {
      // connect the pc
      battery_cap_level = pm->GetBatteryLevel();
      if (battery_cap_level != 6) {
        battery_cap_level = 5;  // usb charging
      }
    } else {
      battery_cap_level = pm->GetBatteryLevel();
    }
    if (p_statusBar->m_current_battery_level != battery_cap_level) {
      p_statusBar->m_current_battery_level = battery_cap_level;
      if ((win_mg->GetCurrentWinID() == WINDOWID_PREVIEW) ||
          (win_mg->GetCurrentWinID() == WINDOWID_INVALID)) {
        // p_statusBar->UpdateBatteryStatus(true, battery_cap_level);
        p_statusBar->UpdateBatteryStatus(false, battery_cap_level);
      } else {
        p_statusBar->UpdateBatteryStatus(false, battery_cap_level);
      }
    }

    sleep(BATTERY_DETECT_TIME);
  }
  return NULL;
}

void StatusBarWindow::UpdateBatteryStatus(bool show) {
  if (show) {
    GetControl("battery_icon")->Hide();
    UpdateBatteryStatus(true, m_current_battery_level);
  } else {
    UpdateBatteryStatus(false, m_current_battery_level);
  }
}

void StatusBarWindow::UpdateBatteryStatus(bool sb, int levelval) {
  View *icon_ = GetControl("battery_icon");
  switch (levelval) {
    case 0:  // 0格
      GraphicView::LoadImage(icon_, "s_top_battery0");
      break;
    case 1:  // 1格
      GraphicView::LoadImage(icon_, "s_top_battery1");
      break;
    case 2:  // 2格
      GraphicView::LoadImage(icon_, "s_top_battery2");
      break;
    case 3:  // 3格
      GraphicView::LoadImage(icon_, "s_top_battery3");
      break;
    case 4:  // DC充电
      GraphicView::LoadImage(icon_, "s_top_charging");
      break;
    case 5:  // USB充电
      GraphicView::LoadImage(icon_, "s_top_usb_charging_only");
      break;
    case 6:  // 4格 满格
      GraphicView::LoadImage(icon_, "s_top_battery4");
      break;
    case 7:  // DC充满
      GraphicView::LoadImage(icon_, "s_top_chargfull");
      break;
    default:
      break;
  }
  if (sb)
    icon_->Show();
  else
    icon_->Hide();
  icon_->Refresh();
}

#endif
