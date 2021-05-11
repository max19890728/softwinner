/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "UIKit/Class/responder.h"

#include "UIKit/Struct/event.h"
#include "UIKit/Struct/touch.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "UI::Responder"

// MARK: 管理觸摸事件

void UI::Responder::TouchesBegan(UITouch const& touch, UIEvent const& event) {
  if (next_) next_->TouchesBegan(touch, event);
}

void UI::Responder::TouchesMoved(UITouch const& touch, UIEvent const& event) {
  if (next_) next_->TouchesMoved(touch, event);
}

void UI::Responder::TouchesEnded(UITouch const& touch, UIEvent const& event) {
  if (next_) next_->TouchesEnded(touch, event);
}

// MARK: 管理按鍵事件

void UI::Responder::PressesBegin(UIPress const& press) {
  if (next_) next_->PressesBegin(press);
}

void UI::Responder::PressesChanged(UIPress const& press) {
  if (next_) next_->PressesChanged(press);
}

void UI::Responder::PressesEnded(UIPress const& press) {
  if (next_) next_->PressesEnded(press);
}
