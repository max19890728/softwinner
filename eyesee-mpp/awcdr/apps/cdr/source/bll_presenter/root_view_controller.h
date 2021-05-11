/* *******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 * *******************************************************************************/

#pragma once

#include "bll_presenter/gui_presenter_base.h"
#include "bll_presenter/presenter.h"
#include "common/subject.h"
class RootViewController : public GUIPresenterBase,
                           public ISubjectWrap(RootViewController),
                           public IObserverWrap(RootViewController),
                           public EyeseeLinux::IPresenter {
 public:
 private:
 public:
  RootViewController();

  virtual ~RootViewController();

  void OnWindowLoaded();

  void OnWindowDetached();

  int HandleGUIMessage(int msg, int val, int id = 0);

  void BindGUIWindow(::Window *win);

  void Update(MSG_TYPE msg, int p_CamID = 0, int p_recordId = 0);

 private:
};
