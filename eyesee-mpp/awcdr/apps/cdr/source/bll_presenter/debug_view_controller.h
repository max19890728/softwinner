/* *****************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 * ****************************************************************************/

#pragma once

#include <signal.h>
#include <string>
#include <vector>

#include "bll_presenter/gui_presenter_base.h"
#include "bll_presenter/presenter.h"
#include "common/subject.h"
#include "window/debug_view.h"

class DebugViewController : public GUIPresenterBase,
                            public ISubjectWrap(DebugViewController),
                            public IObserverWrap(DebugViewController),
                            public EyeseeLinux::IPresenter {
 public:
  DebugView *view_ = nullptr;

 private:
  timer_t data_update_timer_ = nullptr;

 public:
  DebugViewController();

  virtual ~DebugViewController();

  void OnWindowLoaded();

  void OnWindowDetached();

  int HandleGUIMessage(int msg, int val, int id = 0);

  void BindGUIWindow(::Window *win);

  void Update(MSG_TYPE msg, int p_CamID = 0, int p_recordId = 0);

 private:
  static void DataUpdateProc(union sigval sigval);

  void CheckUITestImage();
};
