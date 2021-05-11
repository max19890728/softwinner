/* *******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 * *******************************************************************************/
/**
 * @file    status_bar.h
 * @brief   主畫面狀態視窗 Presenter
 * @author  UnpxreTW
 * @version v0.1
 * @date    2020-07-22
 */

#pragma once

#if 0

#include "bll_presenter/gui_presenter_base.h"
#include "bll_presenter/presenter.h"
#include "common/subject.h"

class StatusBarWindow;
class WindowManager;

namespace EyeseeLinux {

class EventManager;

class StatusBarPresenter : public GUIPresenterBase,
                           public IPresenter,
                           public ISubjectWrap(StatusBarPresenter),
                           public IObserverWrap(StatusBarPresenter) {
 public:
  StatusBarPresenter();

  virtual ~StatusBarPresenter();

  int DeviceModelInit();

  int DeviceModelDeInit();

  void PrepareExit() {}

  int RemoteSwitchRecord() { return 0; }

  int RemoteTakePhoto() { return 0; }

  int GetRemotePhotoInitState(bool &init_state) { return 0; }

  int RemoteSwitchSlowRecord() { return 0; }

  void OnWindowLoaded();

  void OnWindowDetached();

  int HandleGUIMessage(int msg, int val, int id = 0);

  void BindGUIWindow(::Window *win);

  void Update(MSG_TYPE msg, int p_CamID = 0, int p_recordId = 0);

 private:
  WindowManager *win_mg_;
  StatusBarWindow *sb_win_;
  EventManager *em_;
};  // class StatusBarPresenter

}  // namespace EyeseeLinux

#endif
