/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "US363/Updater/updating_view_controller.h"

#include <System/logger.h>
#include <System/updater.h>

#include <thread>

#include "US363/Updater/updating_progress_view.h"

#undef Self
#define Self UpdatingViewController

Self::Self()
    : UI::ViewController::ViewController(),
      logger(System::Logger{std::string{"Updating (VC)"}}),
      updater_(System::Updater::instance()) {}

/* * * * * 繼承類別 * * * * */

// - MARK: UI::ViewController

void Self::Layout(UI::Coder coder) {
  Log(LogLevel::Debug) << '\n';
  this->UI::ViewController::Layout(coder);
  view_->frame(UI::Screen::bounds);
}

void Self::ViewDidLoad() {
  Log(LogLevel::Debug) << '\n';
  this->UI::ViewController::ViewDidLoad();
  view_->background_color_ = UI::Color::black;
  auto start_point = UI::Point(20, 20);
  for (auto const& burn_part : updater_._image_name_list_) {
    auto progress_view = UpdatingProgressView::init(burn_part.second);
    progress_view->frame(
        {origin : start_point, size : UI::Size{width : 200, height : 50}});
    progress_views_[burn_part.first] = progress_view;
    view_->addSubView(progress_view);
    start_point += UI::Point(0, 60);
  }
}

void Self::ViewDidAppear() {
  Log(LogLevel::Debug) << '\n';
  this->UI::ViewController::ViewDidAppear();
  System::Updater::instance().delegate_ = shared_from(this);
  std::thread([] { System::Updater::instance().StartUpdate(); }).detach();
}

void Self::BurningProgress(System::Burntable part, int progress) {
  progress_views_[part]->SetProgress(progress);
}

#undef Self
