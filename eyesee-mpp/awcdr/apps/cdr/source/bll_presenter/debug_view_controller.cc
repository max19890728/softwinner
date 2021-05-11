/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "bll_presenter/debug_view_controller.h"

#include <experimental/filesystem>

#include <algorithm>
#include <string>

#include "common/app_log.h"
#include "common/posix_timer.h"
#include "common/extension/string.h"
#include "window/window_manager.h"

#undef LOG_TAG
#define LOG_TAG "DebugViewController"

namespace filesystem = std::experimental::filesystem;

DebugViewController::DebugViewController() { db_debug(""); }

DebugViewController::~DebugViewController() {
  if (data_update_timer_ != nullptr) {
    stop_timer(data_update_timer_);
    delete_timer(data_update_timer_);
  }
}

void DebugViewController::OnWindowLoaded() {
  auto window_manager = WindowManager::GetInstance();
  view_ = reinterpret_cast<DebugView *>(window_manager->GetWindow(WINDOWID_DEBUG));
  CheckUITestImage();
  create_timer(this, &data_update_timer_, DataUpdateProc);
  stop_timer(data_update_timer_);
  set_period_timer(0, 100000000, data_update_timer_);
}

void DebugViewController::OnWindowDetached() { db_debug(""); }

int DebugViewController::HandleGUIMessage(int msg, int val, int id) {
  db_debug("");
  return 0;
}

void DebugViewController::BindGUIWindow(::Window *win) { this->Attach(win); }

void DebugViewController::Update(MSG_TYPE msg, int p_CamID, int p_recordId) {}

void DebugViewController::DataUpdateProc(union sigval sigval) {
  auto *self = reinterpret_cast<DebugViewController *>(sigval.sival_ptr);
  if (self->view_ != nullptr) self->view_->Refresh();
}

void DebugViewController::CheckUITestImage() {
  std::string path = "/mnt/extsd";
  std::vector<std::string> images_path_ = {};
  for (const auto &entry : filesystem::directory_iterator(path)) {
    filesystem::path image_path = entry.path();
    std::string extension = image_path.extension();
    ToLowerCase(extension);
    if (extension == ".jpg" || extension == ".png") {
      images_path_.push_back(image_path);
    }
    view_->SetTestImagePaths(images_path_);
  }
}
