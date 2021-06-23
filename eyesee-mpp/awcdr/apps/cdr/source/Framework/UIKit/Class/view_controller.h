/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "UIKit/Class/view.h"

namespace UI {

class View;

class Coder;

class ViewController : public std::enable_shared_from_this<UI::ViewController>,
                       public UI::Responder {
 public:
  static inline std::shared_ptr<UI::ViewController> init(UI::Coder decoder = {}) {
    auto building = std::make_shared<UI::ViewController>(decoder);
    return building;
  }

 public:
  enum RectEdge { top, left, right, bottom, not_edge };

  UIView view_;
  std::shared_ptr<UI::ViewController> parent_;
  std::vector<std::shared_ptr<UI::ViewController>> children_;
  UIView title_view_;
  RectEdge pan_start_from_;
  bool is_pan_;

 public:
  explicit ViewController(UI::Coder = {});

  virtual ~ViewController();

  virtual void Layout(UI::Coder);

  virtual void ViewDidLoad();

  virtual void ViewWillAppear();

  virtual void ViewDidAppear();

  virtual void ViewWillDisappear();

  virtual void ViewDidDisappear();

  void AddChild(std::shared_ptr<UI::ViewController> view_controller);

  void RemoveFromParent();

  void Present(
      std::shared_ptr<UI::ViewController>, bool with_animated = true,
      std::function<void()> completion = [] {});

  void Dismiss(
      bool with_animated = true, std::function<void()> completion = [] {});

  /* * * * * 繼承類別 * * * * */
 public:
  // MARK: - UI::Responder

  void PressesEnded(UIPress const&) override;
};
}  // namespace UI

using UIViewController = std::shared_ptr<UI::ViewController>;
