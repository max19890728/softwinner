/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "UIKit/Struct/image.h"

#include <type_traits>
#include <utility>

#include "UIKit/Struct/bundle.h"
#include "common/app_log.h"
#include "common/extension/filesystem.h"

#undef LOG_TAG
#define LOG_TAG "UI::Image"

UI::Bundle const UI::Image::default_bundle = UI::Bundle{defalut_image_path};

UI::Image::Image(std::string path) : image_path_(path) {
  if (path == null_image_path) image_loaded_ = true;
}

UI::Image::~Image() {
  db_info("");
  // @fixme:
  // ::UnloadBitmap(&bitmap_);
  // image_loaded_ = false;
}

BITMAP* UI::Image::Bitmap() {
  if (!image_loaded_) {
    LoadBitmapFromFile(HDC_SCREEN, &bitmap_, image_path_.c_str());
    rect_ = {Point::zero, {width : bitmap_.bmWidth, height : bitmap_.bmHeight}};
    image_loaded_ = true;
  }
  return &bitmap_;
}

void UI::Image::UnloadBitmap() {
  db_info("");
  if (image_loaded_) ::UnloadBitmap(&bitmap_);
  image_loaded_ = false;
}
