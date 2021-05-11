/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <memory>
#include <set>

#include "UIKit/Class/view.h"

namespace UI {

struct Touch;

class ViewController;

class Window : public UI::View {
 public:
  static std::shared_ptr<UI::Window> init() {
    auto building = std::make_shared<UI::Window>();
    building->window_ = building;
    building->layer_->delegate_ = building;
    return building;
  }

  bool is_key_window_;

  Window();

  ~Window();

  void MakeKeyAndVisible();

  LRESULT MessageHandle(HWND, uint, WPARAM, LPARAM);

  auto root_viewcontroller() { return root_viewcontroller_; }

  void root_viewcontroller(std::shared_ptr<UI::ViewController> controller);

  void SendEvent(std::unique_ptr<UI::Event> const& event);

  void SendPress(std::unique_ptr<UI::Press> const& press);

 private:
  pthread_t window_thread_id;
  std::shared_ptr<UI::ViewController> root_viewcontroller_;
};
}  // namespace UI

typedef std::shared_ptr<UI::Window> UIWindow;
