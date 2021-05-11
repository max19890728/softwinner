/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "UIKit/Class/window.h"

#include <Foundation.h>

#include <utility>

#include "UIKit/Class/application.h"
#include "UIKit/Class/view_controller.h"
#include "UIKit/Struct/event.h"
#include "UIKit/Struct/screen.h"
#include "UIKit/Struct/touch.h"
#include "common/app_log.h"
#include "common/thread.h"

#undef LOG_TAG
#define LOG_TAG "UI::Window"

static LRESULT WindowProc(HWND context, uint message, WPARAM wparam,
                          LPARAM lparam) {
  return reinterpret_cast<UI::Window*>(::GetWindowAdditionalData(context))
      ->MessageHandle(context, message, wparam, lparam);
}

UI::Window::Window()
    : UI::View::View(), is_key_window_(false), window_thread_id(NULL) {
  db_info("");
  MAINWINCREATE setting{
    dwStyle : WS_VISIBLE,
    dwExStyle : WS_EX_NONE | WS_EX_TRANSPARENT | WS_EX_AUTOSECONDARYDC,
    spCaption : "Window",
    hMenu : nullptr,
    hCursor : nullptr,
    hIcon : nullptr,
    hHosting : HWND_DESKTOP,
    MainWindowProc : WindowProc,
    lx : UI::Screen::bounds.origin.x,
    ty : UI::Screen::bounds.origin.y,
    rx : UI::Screen::bounds.width(),
    by : UI::Screen::bounds.height(),
    iBkColor : GetWindowElementColor(WE_BGC_WINDOW),
    dwAddData : (DWORD)this
  };
  layer_ = UI::Layer::init(::CreateMainWindow(&setting));
  background_color_ = UI::Color::alpha;
}

UI::Window::~Window() { db_info(""); }

void UI::Window::MakeKeyAndVisible() {
  UI::Application::instance().set_key_window(shared_from(this));
}

LRESULT UI::Window::MessageHandle(HWND window, uint message, WPARAM wparam,
                                  LPARAM lparam) {
  switch (message) {
    case MSG_PAINT: {
      HDC context = ::BeginPaint(window);
      HDC secondary_context = GetSecondaryDC(window);
      layer_->Draw(secondary_context);
      ::BitBlt(secondary_context, Rect2Parameter(frame()), context, 0, 0, 0);
      ReleaseSecondaryDC(window, secondary_context);
      ::EndPaint(window, context);
      break;
    }
    case MSG_LBUTTONDOWN:
    case MSG_MOUSEMOVE:
    case MSG_LBUTTONUP:
      UI::Application::instance().HandleTouch(message, ParamToPoint(lparam));
      break;
    case MSG_MOUSE_FLING:
      db_debug("Fling");
      break;
    case MSG_KEYDOWN:
    case MSG_KEYUP:
      UI::Application::instance().HandlePress(message, wparam);
      break;
    default:
      break;
  }
  return DefaultMainWinProc(window, message, wparam, lparam);
}

void UI::Window::root_viewcontroller(UIViewController new_controller) {
  subviews_.clear();
  addSubView(new_controller->view_);
  root_viewcontroller_ = new_controller;
  root_viewcontroller_->ViewDidLoad();
  root_viewcontroller_->next_ = shared_from_this();
  root_viewcontroller_->ViewWillAppear();  // fixme: 需要在這邊呼叫嗎？
}

void UI::Window::SendEvent(std::unique_ptr<UI::Event> const& event) {
  switch (event->touche_->phase_) {
    case UI::Touch::Phase::begin:
      event->view_->TouchesBegan(event->touche_, event);
      break;
    case UI::Touch::Phase::moved:
      event->view_->TouchesMoved(event->touche_, event);
      break;
    case UI::Touch::Phase::ended:
      event->view_->TouchesEnded(event->touche_, event);
      break;
    default:
      break;
  }
}

void UI::Window::SendPress(UIPress const& press) {
  switch (press->phase_) {
    case UI::Press::Phase::began:
      press->target_->PressesBegin(press);
      break;
    case UI::Press::Phase::ended:
      press->target_->PressesEnded(press);
      break;
    default:
      break;
  }
}
