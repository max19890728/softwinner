/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "UIKit/Component/scroll_view.h"

#include <Foundation.h>

#include "UIKit/Class/view.h"
#include "UIKit/Struct/event.h"
#include "UIKit/Struct/rect.h"
#include "UIKit/Struct/screen.h"
#include "UIKit/Struct/touch.h"
#include "common/app_log.h"
#include "common/posix_timer.h"

#undef LOG_TAG
#define LOG_TAG "UI::ScrollView"

static void InertialScrollProc(union sigval sigval) {
  auto* self = reinterpret_cast<UI::ScrollView*>(sigval.sival_ptr);
  if (self) {
    auto move = self->inertial_velocity_ * 60 / 1000;
    auto last_offset = self->content_offset_;
    self->inertial_velocity_ *= 0.98;
    self->content_offset_ = last_offset - move;
    self->layer_->SetNeedDisplay();
    if (UI::Distance(last_offset, self->content_offset_) * 30 < 5) {
      self->StopInertialScroll();
    }
  }
}

UI::ScrollView::ScrollView()
    : UI::View::View(),
      delegate_(nullptr),
      content_size_(UI::Size::zero),
      content_offset_(UI::Point::zero),
      content_inset_(UI::EdgeInsets::zero),
      content_view_(UI::View::init()),
      last_tracking_point_(UI::Point::zero),
      is_dragging_(false),
      inertial_velocity_(UI::Point::zero),
      velocity_(UI::Point::zero),
      is_scroll_enabled_(true),
      is_paging_enabled_(false),
      is_tracking_(false),
      content_inset_adjustment_behavior_(
          ContentInsetAdjustmentBehavior::never) {
  content_offset_.willSet =
      std::bind(&ScrollView::ContentOffsetWillSet, this, std::placeholders::_1);
  content_offset_.didSet = std::bind(&ScrollView::ContentOffsetDidSet, this);
  content_size_.didSet = std::bind(&ScrollView::ContentSizeDidSet, this);
  content_inset_.didSet = std::bind(&ScrollView::ContentInsetDidSet, this);
}

UI::ScrollView::~ScrollView() {}

// MARK: - 管理內容邊界偏移量與大小

void UI::ScrollView::ContentSizeDidSet() {
  content_view_->frame(UI::Rect{content_offset_, content_size_});
  SetNeedDisplay();
}

auto UI::ScrollView::ContentOffsetWillSet(const UI::Point& new_value)
    -> const UI::Point& {
  if (new_value.y <
      frame().height() - content_size_.height - content_inset_.bottom)
    new_value.y =
        frame().height() - content_size_.height - content_inset_.bottom;
  if (new_value.y > content_inset_.top) new_value.y = content_inset_.top;
  if (new_value.x <
      frame().width() - content_size_.width - content_inset_.right)
    new_value.x = frame().width() - content_size_.width - content_inset_.right;
  if (new_value.x > content_inset_.left) new_value.x = content_inset_.left;
  return new_value;
}

void UI::ScrollView::ContentOffsetDidSet() {
  content_view_->frame(UI::Rect{content_offset_, content_size_});
  SetNeedDisplay();
}

// - MARK: 管理內容插入後的行為

void UI::ScrollView::ContentInsetDidSet() { AdjustedContentInsetDidChange(); }

void UI::ScrollView::AdjustedContentInsetDidChange() {
  switch (content_inset_adjustment_behavior_) {
    case ContentInsetAdjustmentBehavior::never:
      break;
    case ContentInsetAdjustmentBehavior::always:
      content_offset_ -= UI::
      Point{x : content_inset_.left, y : content_inset_.top};
      int test = content_offset_.x;
      db_debug("%d", test);
      SetNeedDisplay();
      break;
  }
}

/* * * * * 繼承類別 * * * * */

// MARK: - UI::Responder

void UI::ScrollView::TouchesBegan(UITouch const& touch, UIEvent const& event) {
  StopInertialScroll();
  inertial_velocity_ = UI::Point::zero;
  is_tracking_ = true;
  last_tracking_point_ = touch->touch_point_;
  last_tracking_time_ = touch->timestamp_;
}

void UI::ScrollView::TouchesMoved(UITouch const& touch, UIEvent const& event) {
  auto touched_point = touch->touch_point_;
  if (!is_dragging_ && UI::Distance(touched_point, last_tracking_point_) < 5) {
    return;
  } else if (!is_dragging_) {
    if (delegate_) delegate_->DidScroll(shared_from(this));
  }
  is_dragging_ = true;
  auto point_move = last_tracking_point_ - touched_point;
  content_offset_ = content_offset_ - point_move;
  last_tracking_point_ = touched_point;
  auto touched_time = touch->timestamp_;
  auto move_time = std::chrono::count_time(last_tracking_time_, touched_time);
  last_tracking_time_ = touch->timestamp_;
  db_msg("point: (%d, %d), time: %lld, offset: (%d, %d)", point_move.x,
         point_move.y, move_time, content_offset_.x, content_offset_.y);
  if (move_time > 0) inertial_velocity_ = point_move * 1000 / move_time;
  layer_->SetNeedDisplay();
}

void UI::ScrollView::TouchesEnded(UITouch const& touch, UIEvent const& event) {
  is_tracking_ = false;
  is_dragging_ = false;
  if (delegate_) delegate_->DidEndDragging(shared_from(this), true);
  layer_->SetNeedDisplay();
  db_msg("Inertial velocity x: %d, y: %d", inertial_velocity_.x,
         inertial_velocity_.y);
  StartInertialScroll();
}

// MARK: - UI::View

// - TODO: 不要把 content_view_ 到 subviews_ 裡面，由 ScrollView
// 掌控他的繪圖週期
void UI::ScrollView::Layout(UI::Coder decoder) {
  this->UI::View::Layout(decoder);
  layer_->AddSublayer(content_view_->layer_);
  content_view_->next_ = shared_from_this();
  content_view_->superview_ = shared_from_this();
}

UIView UI::ScrollView::HitTest(UI::Point point, UIEvent const& event) {
  if (!PointIsInside(point, event)) return nullptr;
  for (auto iter = content_view_->subviews_.rbegin();
       iter != content_view_->subviews_.rend(); ++iter) {
    auto subview = *iter;
    auto on_child = subview->Convert(point, UI::Convert::from);
    if (auto fit_view = subview->HitTest(on_child, event)) return fit_view;
  }
  return this->UI::View::HitTest(point, event);
}

// MARK: - UI::ScrollView

void UI::ScrollView::StartInertialScroll() {
  create_timer(this, &inertial_timer_, InertialScrollProc);
  stop_timer(inertial_timer_);
  set_period_timer(0, 32000000, inertial_timer_);
}

void UI::ScrollView::StopInertialScroll() {
  if (!inertial_timer_) return;
  stop_timer(inertial_timer_);
  delete_timer(inertial_timer_);
  inertial_timer_ = nullptr;
  if (delegate_) delegate_->DidEndDecelerating(shared_from(this));
}

/** MARK - UI::LayerDelegate
 *
 * 使 UI::ScrollView 的渲染機制覆蓋一般 UI::View
 * UI::ScrollView 不完全渲染 `content_view` 的完整內容
 * 而是取得內容的位置與目前可視位置在決定需不需要渲染
 *
 * - note: `content_view` 只由 UI::ScrollView 管理，並不是 `subviews_`
 *         的一部分。
 **/

void UI::ScrollView::Draw(UILayer layer, /* in */ HDC context) {
  this->UI::View::Draw(layer, context);
  // - TODO: 計算可視區域
  for (auto const& content_subview : content_view_->subviews_) {
    content_subview->Draw(content_subview->layer_, context);
  }
}
