/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "UIKit/Component/page_view_controller.h"

UIPageViewController UI::PageViewController::init() {
  auto building = std::make_shared<UI::PageViewController>();
  return building;
}

UI::PageViewController::PageViewController()
    : UI::ViewController::ViewController() {}

void UI::PageViewController::SetViewController(UIViewController controller) {}
