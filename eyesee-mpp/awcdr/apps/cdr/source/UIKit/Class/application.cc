/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "UIKit/Class/application.h"

#include <System/logger.h>
#include <common/extension/set.h>
#include <common/extension/vector.h>
#include <sys/types.h>

#include <chrono>
#include <cmath>
#include <memory>
#include <utility>

#include "UIKit/Class/view_controller.h"
#include "UIKit/Class/window.h"
#include "UIKit/Struct/event.h"
#include "UIKit/Struct/point.h"
#include "UIKit/Struct/screen.h"
#include "UIKit/Struct/touch.h"

UI::Application::Application()
    : logger(System::Logger(std::string{"UI::Application"})) {
  Log(LogLevel::Info) << "Initialization" << '\n';
}

UI::Application::~Application() {
  Log(LogLevel::Info) << "Deinitialization" << '\n';
}

UIWindow UI::Application::get_key_window() {
  return dropFirst<UIWindow>(
      windows_, [&](auto& element) { return element->is_key_window_; });
}

/**
 * 先中斷 MiniGUI 的資料流後將所有 `windows_` 向量內 `UI::Window` 的
 * `is_key_window_` 旗標設定為 `false` 後，如果 `windows_` 內包含此加入的
 * `UI::Window` 則不會再次加入，否則加入 `windows_` 向量的尾端。
 * 將此 `UI::Window` 的 `is_key_window` 設定為 `true`。
 **/
void UI::Application::set_key_window(UIWindow new_window) {
  Terminate();
  for (auto const& window : windows_) window->is_key_window_ = false;
  if (!contains(windows_, new_window)) windows_.push_back(new_window);
  new_window->is_key_window_ = true;
  Run();
}

void UI::Application::Run() {
  if (auto key_window = get_key_window()) {
    MSG message;
    while (::GetMessage(&message, key_window->layer_->context_)) {
      ::TranslateMessage(&message);
      ::DispatchMessage(&message);
    }
  } else {
    Log(LogLevel::Error) << "Miss key_window_" << '\n';
  }
}

void UI::Application::Terminate() {
  if (auto key_window = get_key_window()) {
    ::PostQuitMessage(key_window->layer_->context_);
  }
}

/**
 * 移除事件列表內已經結束的事件，並找到正在移動中的事件或未有移動中的事件則創建新
 * 事件並對使用中的事件進行 Hit Test 與設定時戳，在使用 `SendEvent()`。
 **/
void UI::Application::HandleTouch(uint state, UI::Point point) {
  removeAll<UIEvent>(events_, [&](auto& element) {
    return element->touche_->phase_ == UI::Touch::Phase::ended;
  });
  auto changing = dropFirst<UIEvent>(events_, [&](auto& element) {
    return element->touche_->phase_ != UI::Touch::Phase::ended;
  });
  if (changing == events_.end()) {
    changing = events_.emplace(UI::Event::init(point)).first;
  }
  if (auto key_window = get_key_window()) {
    auto& now_event = *changing;
    if (now_event->touche_->phase_ == UI::Touch::Phase::begin) {
      auto fit_view = key_window->HitTest(point, now_event);
      now_event->SetKeyWindow(key_window);
      now_event->SetFirstResponder(fit_view);
    }
    now_event->timestamp(std::chrono::system_clock::now());
    now_event->SetPoint(point);
    now_event->SetState(state);
    SendEvent(now_event);
#if 0
    for (auto const& event : events_) {
      Log(LogLevel::Debug) << event->touche_->touch_point_ << '\n';
    }
#endif
  }
}

void UI::Application::HandlePress(uint state, WPARAM key_number) {
  removeAll<UIPress>(press_, [&](auto& element) {
    return element->phase_ == UI::Press::Phase::ended;
  });
  auto changing = dropFirst<UIPress>(press_, [&](auto& element) {
    return element->phase_ != UI::Press::Phase::ended;
  });
  if (changing == press_.end()) {
    changing = press_.emplace(UI::Press::init(key_number)).first;
  }
  if (auto key_window = get_key_window()) {
    auto& now_press = *changing;
    now_press->set_phase(state);
    now_press->window_ = key_window;
    auto send_to = key_window->root_viewcontroller();
    while (!send_to->children_.empty()) {
      send_to = send_to->children_.back();
    }
    now_press->target_ = send_to;
    SendPress(now_press);
  }
}

void UI::Application::SendEvent(UIEvent const& event) {
  if (auto send_to = event->window_) send_to->SendEvent(event);
}

void UI::Application::SendPress(UIPress const& press) {
  if (auto send_to = press->window_) send_to->SendPress(press);
}
