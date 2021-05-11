/******************************************************************************
 Copyright (c), 2020, Ultracker Tech. All rights reserved.

 @filename: scroll_selector.h
 @author UnpxreTW
 @version v0.1
 @date 2020-07-22

 @history:
*******************************************************************************/

#pragma once

#include <string>
#include "widgets/system_widget.h"

class ScrollSelector : public SystemWidget {
  DECLARE_DYNCRT_CLASS(ScrollSelector, Runtime)

 private:
  BITMAP normal_image_;

 public:
  ScrollSelector(View *parent);

  virtual ~ScrollSelector();

  virtual void GetCreateParams(CommonCreateParams &params);

  virtual int HandleMessage(HWND hwnd, int message, WPARAM wparam,
                            LPARAM lparam);

  virtual void setImage(const std::string &path);

  static void loadImage(View *ctrl, const char *name);
};
