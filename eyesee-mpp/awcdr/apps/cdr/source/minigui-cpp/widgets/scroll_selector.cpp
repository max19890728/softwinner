/******************************************************************************
 Copyright (c), 2020, Ultracker Tech. All rights reserved.

 @filename: scroll_selector.cpp
 @author UnpxreTW
 @version v0.1
 @date 2020-07-22

 @history:
*******************************************************************************/

#include "widgets/scroll_selector.h"
#include <string>
#include "resource/resource_manager.h"

#undef LOG_TAG
#define LOG_TAG "ScrollSelector"

IMPLEMENT_DYNCRT_CLASS(ScrollSelector)

ScrollSelector::ScrollSelector(View *parent) : SystemWidget(parent) {}

ScrollSelector::~ScrollSelector() {}

void ScrollSelector::GetCreateParams(CommonCreateParams &params) {
  params.class_name = CTRL_SCROLLWND;
  params.alias = GetClassName();
  params.style = WS_VISIBLE | SS_NOTIFY | SS_BITMAP | SS_CENTERIMAGE;
  params.exstyle = WS_EX_USEPARENTFONT;
  params.x = 0;
  params.y = 0;
  params.w = DEFAULT_CTRL_WIDTH;
  params.h = DEFAULT_CTRL_HEIGHT;
}

int ScrollSelector::HandleMessage(HWND hwnd, int message, WPARAM wparam,
                                  LPARAM lparam) {
  return SystemWidget::HandleMessage(hwnd, message, wparam, lparam);
}

void ScrollSelector::loadImage(View *ctrl, const char *name) {
  ScrollSelector *view = reinterpret_cast<ScrollSelector *>(ctrl);
  if (view) {
    std::string path = R::get()->GetImagePath(name);
    view->setImage(path);
  }
}

void ScrollSelector::setImage(const std::string &path) {
  if (path.empty()) return;
  if (normal_image_.bmBits) UnloadBitmap(&normal_image_);
  ::LoadBitmapFromFile(HDC_SCREEN, &normal_image_, path.c_str());
  SendMessage(GetHandle(), STM_SETIMAGE, (WPARAM)&normal_image_, 0);
}
