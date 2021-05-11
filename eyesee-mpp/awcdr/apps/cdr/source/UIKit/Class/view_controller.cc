/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "UIKit/Class/view_controller.h"

#include <Foundation.h>

#include <string>

#include "UIKit/Component/label.h"
#include "UIKit/buildable.h"
#include "UIKit/decoder.h"
#include "common/app_log.h"
#include "common/extension/vector.h"

#undef LOG_TAG
#define LOG_TAG "UI::ViewController"

UI::ViewController::ViewController(UI::Coder decoder)
    : view_(UI::View::init(decoder)),
      parent_(nullptr),
      title_view_(UI::View::init()),  // 佔位視圖，需要確保視圖順序。
      pan_start_from_(UI::ViewController::RectEdge::not_edge),
      is_pan_(false) {}

UI::ViewController::~ViewController() { db_info(""); }

void UI::ViewController::Layout(UI::Coder decoder) {
#if 0
  view_->Layout(decoder);
  if (decoder.layout_.contains("title_view")) {
    auto title_view_layout = decoder.layout_.at("title_view");
    auto title_view = UI::View::init(UI::Decoder{title_view_layout});
    this->title_view_ = title_view;
  } else {
    title_view_->IsHidden(true);
  }
#endif
  // view_->addSubView(title_view_);
}

void UI::ViewController::ViewDidLoad() {
  db_info("");
  view_->next_ = shared_from_this();
  view_->SetNeedDisplay();
}

void UI::ViewController::ViewWillAppear() {
  for (auto const &view : view_->subviews_) {
    // view->set_hidden(false);
  }
  db_info("");
  ViewDidAppear();
}

void UI::ViewController::ViewDidAppear() {
  for (auto const &view : view_->subviews_) {
    view->layer_->LayoutIfNeed();
  }
  db_info("");
}

void UI::ViewController::ViewWillDisappear() {
  for (auto const &view : view_->subviews_) {
    for (auto const &controller : children_) {
      // view->set_hidden(view != controller->view_);
    }
  }
  ViewDidDisappear();
}

void UI::ViewController::ViewDidDisappear() { db_info(""); }

void UI::ViewController::AddChild(
    std::shared_ptr<UI::ViewController> view_controller) {
  view_controller->parent_ = shared_from(this);
  view_controller->next_ = shared_from(this);
  children_.push_back(view_controller);
}

void UI::ViewController::RemoveFromParent() {
  view_->layer_->RemoveFromSuperlayer();
  removeElementIn(parent_->view_->subviews_, view_);
  removeElementIn(parent_->children_, shared_from(this));
}

void UI::ViewController::Present(
    std::shared_ptr<UI::ViewController> view_controller, bool with_animated,
    std::function<void()> completion) {
  view_->addSubView(view_controller->view_);
  view_controller->view_->next_ = view_controller;
  view_controller->ViewDidLoad();
  AddChild(view_controller);
  view_controller->parent_ = shared_from(this);
  view_controller->ViewWillAppear();
  ViewWillDisappear();
  completion();
}

void UI::ViewController::Dismiss(bool with_animated,
                                 std::function<void()> completion) {
  parent_->ViewWillAppear();
  ViewWillDisappear();
  RemoveFromParent();
  completion();
}

void UI::ViewController::PressesEnded(UIPress const &press) {
  if (press->key_.key_code_ == UI::KeybordHIDUsage::back) {
    if (parent_) {
      Dismiss();
    }
  }
}
