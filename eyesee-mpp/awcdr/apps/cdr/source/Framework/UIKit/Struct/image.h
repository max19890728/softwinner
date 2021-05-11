/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <common/app_log.h>
#include <common/extension/filesystem.h>

#include <memory>
#include <string>

#include "UIKit/Struct/bundle.h"
#include "UIKit/Struct/rect.h"
#include "data/gui.h"

static const std::string &defalut_image_path = "/usr/share/minigui/res/images/";
static const std::string &null_image_path = defalut_image_path + "null.png";

namespace UI {

struct Image {
 public:
  static Bundle const default_bundle;

  static std::shared_ptr<UI::Image> init(std::string image_name = "", std::string inDirectory = "",
                                         Bundle withBundle = UI::Image::default_bundle) {
    auto path = withBundle.Path(image_name, "png", inDirectory);
    if (!filesystem::exists(path)) db_error("File %s Not find", path.c_str());
    return std::make_shared<UI::Image>(image_name.empty() ? null_image_path : path);
  }

 private:
  bool image_loaded_ = false;
  BITMAP bitmap_;

 public:
  std::string image_path_;
  UI::Rect rect_ = UI::Rect::zero;

  explicit Image(std::string path);

  ~Image();

  BITMAP *Bitmap();

  void UnloadBitmap();
};

inline void Load(std::shared_ptr<UI::Image> const &image, HDC const &context, UI::Rect rect) {
  auto bitmap = image->Bitmap();
  UI::Rect paint_rect{rect.origin, image->rect_.size};
  if (image->rect_.width() < rect.width()) {
    paint_rect.origin.x += (rect.width() - image->rect_.width()) / 2;
  }
  if (image->rect_.height() < rect.height()) {
    paint_rect.origin.y += (rect.height() - image->rect_.height()) / 2;
  }
  FillBoxWithBitmap(context, Rect2Parameter(paint_rect), bitmap);
}
}  // namespace UI

typedef std::shared_ptr<UI::Image> UIImage;
