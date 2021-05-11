/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <Foundation.h>

#include <memory>
#include <set>

#include "UIKit/Class/view.h"

namespace UI {

class Control : public UI::View {
  // MARK: - 初始化器
 public:
  Control();

  // MARK: - 控件屬性
 public:
  enum class State { normal, highlighted, disabled, selected };

  Variable<State> state_;

  // MARK: - 控件的目標與動作
 public:
  enum class Event { touch_dwon, touch_up_inside, value_changed };

  typedef std::function<void(std::shared_ptr<Control>)> Action;
  typedef std::pair<Control::Event, Action> ActionPair;

  void AddTarget(Any, Action, Control::Event);

  void RemoveTarget(Any, Action, Control::Event);

  std::vector<Action> ActionsFor(Any, Control::Event);

  virtual std::vector<Control::Event> AllControlEvents() = 0;

  std::set<Any> AllTargets();

 private:
  std::map<Any, std::vector<ActionPair>> target_table;

  // MARK: - 觸發操作
 public:
  void SendActoion(UI::Control::Event);

  // MARK: - 跟蹤觸控事件
 public:
  bool is_touch_inside_;

  /* * * * * 繼承類別 * * * * */

  // MARK: - UI::Responder

 public:
  void TouchesBegan(UITouch const &, UIEvent const &) override;

  void TouchesMoved(UITouch const &, UIEvent const &) override;

  void TouchesEnded(UITouch const &, UIEvent const &) override;

  /* * * * * 其他成員 * * * * */
};
}  // namespace UI

typedef std::shared_ptr<UI::Control> UIControl;
