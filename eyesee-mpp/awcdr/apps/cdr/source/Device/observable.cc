/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/observable.h"

#include <Foundation.h>

#include <algorithm>

Device::Observable::Observable() {}

void Device::Observable::AddTarget(Any target, Action action, Event event) {
  auto target_exist = target_table_.find(target);
  auto action_pair = std::make_pair(event, action);
  if (target_exist == target_table_.end()) {
    std::vector<ActionPair> action_list;
    action_list.push_back(action_pair);
    target_table_[target] = action_list;
  } else {
    target_table_[target].push_back(action_pair);
  }
}

auto Device::Observable::ActionFor(Any target, Event event)
    -> std::vector<Action> {
  std::vector<Action> result;
  auto iter = target_table_.find(target);
  if (iter != target_table_.end()) {
    auto action_list = target_table_.at(target);
    for (auto action_pair : action_list) {
      if (action_pair.first == event) result.push_back(action_pair.second);
    }
  }
  return result;
}

void Device::Observable::RemoveTarget(Any target, Action action, Event event) {
  if (target == nullptr) {
    target_table_.clear();
  } else {
    auto remove_target = target_table_.find(target);
    if (action == nullptr and event == Event::all_event) {
      target_table_.erase(remove_target);
    } else {
      auto& action_list = remove_target->second;
      // @fixme: 目前無法比較 std::function
      std::remove_if(action_list.begin(), action_list.end(), [&](auto item) {
        if (event == Event::all_event) {
          return true;
        } else {
          return item.first == event;
        }
      });
    }
  }
}

auto Device::Observable::AllTargets() -> std::set<Any> {
  std::set<Any> result;
  for (auto target_pair : target_table_) {
    result.insert(target_pair.first);
  }
  return result;
}

void Device::Observable::SendAction(Event event) {
  for (auto target : AllTargets()) {
    for (auto action : ActionFor(target, event)) {
      action(this);
    }
  }
}
