/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <memory>

#include "UIKit/Class/view.h"

namespace UI {

class ImageView : public UI::View {
  // MARK: - 初始化器
 public:
  static inline std::shared_ptr<UI::ImageView> init() {
    auto building = std::make_shared<UI::ImageView>();
    building->Layout();
    return building;
  }

  static inline std::shared_ptr<UI::ImageView> init(UIImage image) {
    auto building = UI::ImageView::init();
    building->image(image);
    return building;
  }

  static inline std::shared_ptr<UI::ImageView> init(UIImage image, UIImage highlight_image) {
    auto building = UI::ImageView::init(image);
    building->highlight_image(highlight_image);
    return building;
  }

  ImageView();

  // MARK: - 顯示的圖像
 private:
  UIImage image_;
  UIImage highlight_image_;

 public:
  auto image() const { return image_; }

  void image(UIImage);

  auto highlight_image() const { return highlight_image_; }

  void highlight_image(UIImage);

  // MARK: - 視圖配置

  bool is_highlighted_;

  void SetHighlight(bool);

  /* * * * * 繼承類別 * * * * */

  // MARK: - UI::LayerDelegate

 public:
  void Draw(UILayer, HDC) override;
};
}  // namespace UI

typedef std::shared_ptr<UI::ImageView> UIImageView;
