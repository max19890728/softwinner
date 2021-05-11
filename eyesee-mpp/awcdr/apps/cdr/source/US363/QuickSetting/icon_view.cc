/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "US363/QuickSetting/icon_view.h"

#undef Self
#define Self QuickIconView

#undef LOG_TAG
#define LOG_TAG "QuickIconView"

Self::Self(std::function<void(int)> callback)
    : UI::ImageView::ImageView(), callback_(callback) {}

/* * * * * 繼承類別 * * * * */

// MARK: - UI::Responder

void Self::TouchesBegan(UITouch const& touch, UIEvent const& event) {
  callback_(tag_);
  this->UI::Responder::TouchesBegan(touch, event);
}

#undef LOG_TAG
#undef Self
