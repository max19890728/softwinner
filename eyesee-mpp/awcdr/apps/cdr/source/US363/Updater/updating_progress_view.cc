/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "US363/Updater/updating_progress_view.h"

#undef Self
#define Self UpdatingProgressView

Self::Self(std::string name)
    : UI::View::View(),
      progress_bar_(UI::View::init()),
      name_label_(UI::Label::init()) {
  name_label_->text_ = name;
}

/* * * * * 其他成員 * * * * */

void Self::SetProgress(int progress) {
  auto width = static_cast<double>(progress) / 100 * 180;
  progress_bar_->frame(UI::Rect{
    origin : UI::Point{x : 10, y : 5},
    size : UI::Size{width : static_cast<int>(width), height : 20}
  });
}

/* * * * * 繼承類別 * * * * */

// - MARK: UI::View

void Self::Layout(UI::Coder) {
  this->UI::View::Layout();
  auto progress_bar_background = UI::View::init();
  progress_bar_background->frame(UI::Rect{
    origin : UI::Point{x : 10, y : 5},
    size : UI::Size{width : 180, height : 20}
  });
  progress_bar_background->background_color_ = UI::Color::white;
  addSubView(progress_bar_background);
  progress_bar_->frame(UI::Rect{
    origin : UI::Point{x : 10, y : 5},
    size : UI::Size{width : 0, height : 20}
  });
  progress_bar_->background_color_ = UI::Color{0xFF00FF00};
  addSubView(progress_bar_);
  name_label_->frame(UI::Rect{
    origin : UI::Point{x : 10, y : 30},
    size : UI::Size{width : 180, height : 15}
  });
  name_label_->text_color_ = UI::Color::white;
  addSubView(name_label_);
}

#undef Self
