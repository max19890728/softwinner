/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include "UIKit/Control/control.h"

#include <Foundation.h>

UI::Control::Control()
    : UI::View::View(),
      is_touch_inside_(false),
      state_(Control::State::normal) {}

void UI::Control::AddTarget(Any target, UI::Control::Action action,
                            Control::Event event) {
  auto target_exist = target_table.find(target);
  auto action_pair = std::make_pair(event, action);
  if (target_exist == target_table.end()) {
    std::vector<ActionPair> action_list;
    action_list.push_back(action_pair);
    target_table[target] = action_list;
  } else {
    target_table[target].push_back(action_pair);
  }
}

void UI::Control::RemoveTarget(Any target, UI::Control::Action action,
                               Control::Event event) {}

std::vector<UI::Control::Action> UI::Control::ActionsFor(Any target,
                                                         Control::Event event) {
  std::vector<UI::Control::Action> result;
  auto iter = target_table.find(target);
  if (iter != target_table.end()) {
    auto action_list = target_table.at(target);
    for (auto action_pair : action_list) {
      if (action_pair.first == event) result.push_back(action_pair.second);
    }
  }
  return result;
}

std::set<Any> UI::Control::AllTargets() {
  std::set<Any> result;
  for (auto target_pair : target_table) {
    result.insert(target_pair.first);
  }
  return result;
}

// MARK: - 觸發操作

void UI::Control::SendActoion(UI::Control::Event event) {
  for (auto target : AllTargets()) {
    for (auto action : ActionsFor(target, event)) {
      action(shared_from(this));
    }
  }
}

/* * * * * 繼承類別 * * * * */

// MARK: - UI::Responder

void UI::Control::TouchesBegan(UITouch const& touch, UIEvent const& event) {
  SendActoion(UI::Control::Event::touch_dwon);
  is_touch_inside_ = true;
}

void UI::Control::TouchesMoved(UITouch const& touch, UIEvent const& event) {}

void UI::Control::TouchesEnded(UITouch const& touch, UIEvent const& event) {
  if (is_touch_inside_) SendActoion(Control::Event::touch_up_inside);
  is_touch_inside_ = false;
}
