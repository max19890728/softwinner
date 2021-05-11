/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <memory>

#include "UIKit/Class/view_controller.h"

namespace UI {

class PageViewControllerDelegate {};

class PageViewControllerDataSource {
  UIViewController ViewControllerBefore();

  UIViewController ViewControllerAfter();
};

class PageViewController : public UI::ViewController {
 public:
  static std::shared_ptr<UI::PageViewController> init();

  std::shared_ptr<UI::PageViewControllerDataSource> data_source_;

  PageViewController();

  void SetViewController(UIViewController);
};
}  // namespace UI

typedef std::shared_ptr<UI::PageViewController> UIPageViewController;
