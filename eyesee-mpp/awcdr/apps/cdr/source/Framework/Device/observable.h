/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <Foundation.h>

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <vector>

namespace Device {

class Observable {
 public:
  Observable();

  enum class Event { value_change, all_event };

  typedef std::function<void(Any)> Action;
  typedef std::pair<Event, Action> ActionPair;

  void AddTarget(Any, Action, Event);

  std::vector<Action> ActionFor(Any, Event);

  void RemoveTarget(Any = nullptr, Action = nullptr, Event = Event::all_event);

  std::set<Any> AllTargets();

  void SendAction(Event);

 private:
  std::map<Any, std::vector<ActionPair>> target_table_;
};
}  // namespace Device
