/*****************************************************************************
 *
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: container.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:
    ContainerWidget has the functionality to notify all children.
 History:
*****************************************************************************/

#pragma once

#include "widgets/widgets.h"

class ContainerWidget : public Widget {
 public:
  explicit ContainerWidget(View *parent);

  virtual ~ContainerWidget();

  virtual int HandleMessage(HWND hwnd, int message, WPARAM wparam,
                            LPARAM lparam);

  virtual int NotifyChildren(int iMsg, WPARAM wparam, LPARAM lparam);

 public:
  fastdelegate::FastDelegate0<> OnShow;
  fastdelegate::FastDelegate0<> OnHide;
};
