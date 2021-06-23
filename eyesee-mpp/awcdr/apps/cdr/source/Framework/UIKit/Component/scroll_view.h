/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <Foundation.h>

#include <memory>

#include "UIKit/Class/view.h"
#include "UIKit/Struct/edge_insets.h"
#include "UIKit/Struct/event.h"
#include "UIKit/Struct/touch.h"
#include "common/app_log.h"

namespace UI {

class ScrollView;

class ScrollViewDelegate {
#undef LOG_TAG
#define LOG_TAG "UI::ScrollViewDelegate"
 public:
  virtual void DidScroll(std::shared_ptr<UI::ScrollView>) { db_info(""); }

  virtual void DidEndDragging(std::shared_ptr<UI::ScrollView>, /* will */ bool decelerate) {
    db_info("");
  }

  virtual void DidEndDecelerating(std::shared_ptr<UI::ScrollView>) { db_info(""); }
#undef LOG_TAG
};

class ScrollView : public UI::View {
 public:
  static inline auto init() {
    auto building = std::make_shared<UI::ScrollView>();
    building->Layout();
    return building;
  }

  std::shared_ptr<UI::ScrollViewDelegate> delegate_;
  UIView content_view_;
  UI::Point last_tracking_point_;
  std::chrono::system_clock::time_point last_tracking_time_;
  bool is_dragging_;
  UI::Point inertial_velocity_;  // 滾動慣性速度，單位：像素/每秒

 protected:
  UI::Point velocity_;
  bool is_scroll_enabled_;
  bool is_paging_enabled_;
  bool is_tracking_;

 private:
  timer_t inertial_timer_ = nullptr;

 public:
  ScrollView();

  ~ScrollView();

  // MARK: - 管理內容邊界偏移量與大小

 public:
  UI::Size content_size_;

  UI::Point content_offset_;

 private:
  virtual void ContentSizeDidSet();

  virtual auto ContentOffsetWillSet(const UI::Point &) -> const UI::Point &;

  virtual void ContentOffsetDidSet();

  // - MARK: 管理內容安全距離與行為

 public:
  UI::EdgeInsets content_inset_;

  // 安全區域插入後調整內容位置的行為
  enum class ContentInsetAdjustmentBehavior {
    never,  // 更改安全距離後不會更改可見範圍
    always  // 可視範圍會加入安全區域來計算
  };

  ContentInsetAdjustmentBehavior content_inset_adjustment_behavior_;

 private:
  void ContentInsetDidSet();

  // 當調整安全區域後調用此方法
  virtual void AdjustedContentInsetDidChange();

  /* * * * * 繼承類別 * * * * */

 public:
  /**  MARK: - UI::Responder
   * - note: UI::ScrollView
   * 並不會將事件繼續往響應者鏈傳送，如果需要繼續傳送則需要覆寫 UI::Responder
   * 相關方法。
   **/

  void TouchesBegan(UITouch const &, UIEvent const &) override;

  void TouchesMoved(UITouch const &, UIEvent const &) override;

  void TouchesEnded(UITouch const &, UIEvent const &) override;

  // MARK: - UI::View

  void Layout(UI::Coder = {}) override;

  // note: 因 `content_view_` 不包含在 subviews_ 內，所以必須自行實現
  // `HitTest` 以對 `content_view_` 的子視圖進行 `HitTest`。
  UIView HitTest(UI::Point, UIEvent const &) override;

  // MARK: - UI::ScrollView

  void StartInertialScroll();

  void StopInertialScroll();

  // MARK: - UI::LayerDelegate

  void Draw(UILayer, /* in */ HDC) override;
};
}  // namespace UI

typedef std::shared_ptr<UI::ScrollView> UIScrollView;
