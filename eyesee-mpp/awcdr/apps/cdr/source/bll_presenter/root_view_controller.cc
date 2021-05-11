/* *******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 * *******************************************************************************/

#include "bll_presenter/root_view_controller.h"

#include "window/window_manager.h"

#undef LOG_TAG
#define LOG_TAG "RootViewController"

RootViewController::RootViewController() { db_debug(""); }

RootViewController::~RootViewController() { db_debug(""); }

void RootViewController::OnWindowLoaded() { db_debug(""); }

void RootViewController::OnWindowDetached() { db_debug(""); }

int RootViewController::HandleGUIMessage(int msg, int val, int id) {
  db_debug("");
  return 0;
}

void RootViewController::BindGUIWindow(::Window *win) { this->Attach(win); }

void RootViewController::Update(MSG_TYPE msg, int p_CamID, int p_recordId) {}
