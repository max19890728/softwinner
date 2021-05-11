/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <Foundation.h>

#include <memory>
#include <set>
#include <vector>

#include "UIKit/Class/layer.h"
#include "UIKit/Class/responder.h"
#include "UIKit/Struct/color.h"
#include "UIKit/Struct/image.h"
#include "UIKit/Struct/rect.h"
#include "UIKit/buildable.h"
#include "UIKit/decoder.h"

namespace UI {

class Window;

class View : public std::enable_shared_from_this<UI::View>,
             public UI::Responder,
             public UI::LayerDelegate {
  // MARK: - 初始化器

 public:
  static inline std::shared_ptr<UI::View> init(UI::Coder decoder = {}) {
    auto building = std::make_shared<UI::View>();
    building->Layout(decoder);
    return building;
  }

  static inline std::shared_ptr<UI::View> init(UI::Rect frame) {
    auto building = UI::View::init();
    building->frame(frame);
    return building;
  }

  View();

  // - MARK: 視圖渲染管理

 public:
  /**
   * 當此值被設定完成時會呼叫
   * `private: virtual void BackgroundColorDidSet();`
   **/
  UI::Color background_color_;

 private:
  /**
   *  當 `background_color_` 被修改完後會自動呼叫此方法，覆寫此方法以改變預設行為
   **/
  virtual void BackgroundColorDidSet();

 public:
  auto IsHidden() { return layer_->IsHidden(); }

  void IsHidden(bool);

  /**
   * 確認繪製圖層時需不需要清除原本的內容
   *
   * - note: 尚未使用
   **/
  bool clears_context_before_drawing_;

  /**
   * 建構視圖所需要的圖層類，子類可以覆寫此方法來使用自己需要的圖層類
   **/
  virtual UILayer LayerClass();

  auto Layer() { return layer_; }

  // todo: 隱藏成員。
  UILayer layer_;

  // MAKR: - 事件處理旗標

  // 是否要接收與處理來自響應鏈的消息，預設為 `true`
  bool is_user_interaction_enable_;

  // 是否將事件在響應鏈中攔截不繼續向下傳遞，預設為 `false`
  bool is_exclusive_touch_;

  // MARK: - 邊界與框架範圍配置

 public:
  UI::Rect frame_;

  auto frame() const -> const UI::Rect&;

  void frame(UI::Rect const& new_frame);

 private:
  virtual auto FrameGet() const -> const UI::Rect&;

  virtual void FrameDidSet();

  // MARK: - 視圖層次結構管理

 public:
  // 本視圖的父視圖，如果沒有則為 `nullptr`
  std::shared_ptr<UI::View> superview_;
  // 本視圖的所有子視圖序列容器
  std::vector<std::shared_ptr<UI::View>> subviews_;
  // 擁有本視圖的 `UI::Window` 對象，不存在則為 `nullptr`
  std::shared_ptr<UI::Window> window_;

  // 將視圖添加到子視圖序列的尾端，並同時加入到 `UI::Layer` 渲染鏈
  void addSubView(std::shared_ptr<UI::View> view);

  void RemoveFromSuperView();

  // 移動指定的子視圖到序列最尾端
  void BringSubviewToFront(std::shared_ptr<UI::View>);

  // MARK: - 繪製與更新視圖

  void SetNeedDisplay();

  // MARK: - 在運行時識別視圖

  int tag_;

  // MARK: - 不同座標系間的轉換

 public:
  /**
   * 根據 `ConvertFlag`轉換為目標視圖座標係的座標或是從目標
   * 視圖座標系轉換為本視圖座標系。
   *
   * - Note: 目標必須屬於同一個 `UI::Window` 視圖樹內。
   *         如果目標視圖為 `nullptr` 則使用 `UI::Window` 為目標。
   *         如果目標視圖不為同一個 `UI::Window` 視圖樹則為未定義行為。
   **/
  UI::Point Convert(UI::Point, UI::Convert, std::shared_ptr<UI::View> const& = nullptr);

  /**
   * 根據 `ConvertFlag`轉換為目標視圖的範圍或是從目標視圖的
   * 範圍轉換為本視圖範圍。
   *
   * - Note: 目標必須屬於同一個 `UI::Window` 渲染樹內。
   *         如果目標視圖為 `nullptr` 則使用 `UI::Window` 為目標。
   *         如果目標視圖不為同一個 `UI::Window` 渲染樹則為未定義行為。
   **/
  UI::Rect Convert(UI::Rect, UI::Convert, std::shared_ptr<UI::View> const& = nullptr);

  // MARK: - 視圖命中測試

 public:
  virtual std::shared_ptr<UI::View> HitTest(UI::Point, UIEvent const&);

  bool PointIsInside(UI::Point, UIEvent const&);

  // MARK: - 其他方法

 public:
  virtual void Layout(UI::Coder decoder = {});

  /* * * * * 繼承類別 * * * * */

  // - MARK: UI::Responder

  void TouchesBegan(UITouch const&, UIEvent const&) override;

  void TouchesMoved(UITouch const&, UIEvent const&) override;

  void TouchesEnded(UITouch const&, UIEvent const&) override;

  // - MARK: UI::LayerDelegate

  void Draw(UILayer, /* in */ HDC) override;
};
}  // namespace UI

using UIView = std::shared_ptr<UI::View>;
