/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <System/logger.h>
#include <sys/types.h>

#include <memory>
#include <set>
#include <vector>

#include "UIKit/Class/responder.h"
#include "UIKit/Class/window.h"
#include "UIKit/Struct/event.h"

namespace UI {

class ApplicationDelegate {};

/**
 * `UI::Application` 負責掌管應用程式的生命週期
 * 大多數情況不需要繼承此類，只使用單例模式。
 *
 * * 處理從 `MiniGUI` 事件流傳入的事件，尋找觸控事件經過的視圖物件，
 * 並將事件分配到觸發的 Key Window 內。
 * * 維護一個 `UI::Window` 陣列，可以訪問所有 `UI::View` 對象。
 **/
class Application : public std::enable_shared_from_this<UI::Application>,
                    public UI::Responder {
  // MARK: - 單例模式使用

 public:
  static Application& instance() {
    static Application instance_;
    return instance_;
  }

 private:
  Application();
  ~Application();
  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;

  // MARK: - -
 private:
  System::Logger logger;

  std::set<std::unique_ptr<UI::Event>> events_;
  std::set<std::unique_ptr<UI::Press>> press_;

 public:
  std::vector<std::shared_ptr<UI::Window>> windows_;

  void delegate(std::unique_ptr<UI::ApplicationDelegate> const&);

  // 遍歷 `windows_` 找出 `is_key_window_` 為 `true` 的 `UI::Window`
  // ，如果未找到則結果為 `nullptr`
  std::shared_ptr<UI::Window> get_key_window();

  // 將 `UI::Window` 加入 `windows_` 向量內並設定為 Key Window。
  void set_key_window(std::shared_ptr<UI::Window> new_window);

  // 將 MiniGUI Event 轉換為 `UI::Event` 並進行 Hit Test
  // 找到適合的圖層樹並使用 `UI::Application::SendEvent()` 發送事件。
  void HandleTouch(uint state, UI::Point point);

  // 將 MiniGUI Key Event 轉換為 `UI::Press` 並找到適合的事件樹並使
  // 用 `UI::Application::SendPress()` 發送事件。
  void HandlePress(uint state, WPARAM key_number);

 private:
  // 開始將 MiniGUI 的資料流轉換至 App 的資料流。
  void Run();

  // 停止將 MiniGUI 的資料流轉換至 App 的資料流。
  void Terminate();

  // 將 `UI::Event` 發送至找到的 Key Window 內。
  void SendEvent(std::unique_ptr<UI::Event> const&);

  void SendPress(std::unique_ptr<UI::Press> const&);
};
}  // namespace UI
