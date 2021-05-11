/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "UIKit/Struct/color.h"
#include "UIKit/Struct/image.h"
#include "UIKit/Struct/rect.h"

namespace UI {

class Layer;

class LayerDelegate {
 public:
  virtual void Draw(std::shared_ptr<UI::Layer>, HDC) = 0;
};

class Layer : public std::enable_shared_from_this<UI::Layer> {
  // MARK: - 初始化器
 public:
  static inline std::shared_ptr<UI::Layer> init() { return std::make_shared<UI::Layer>(); }

  // 只用於 UI::Window 以包含 Context 作為繪圖樹的起點;
  static inline std::shared_ptr<UI::Layer> init(HWND context) {
    auto initialize = UI::Layer::init();
    initialize->context_ = context;
    RECT rect;
    if (::GetWindowRect(context, &rect)) {
      initialize->frame_ = UI::Rect::init(rect);
    }
    return initialize;
  }

  Layer();

  // MARK: -
 public:
  std::weak_ptr<LayerDelegate> delegate_;
  HWND context_;
  UIImage content_;

  void AddSublayer(std::shared_ptr<UI::Layer>);

  void RemoveFromSuperlayer();

  void UnloadContent();

  // - MARK: Layout Function

  void SetNeedLayout();

  void LayoutIfNeed();

  // - MARK: Get Function

  UI::Color const& BackgroundColor();

  bool IsHidden();

  // - MARK: Set Function

  void SetBackground(UI::Color const&);

  void SetContent(UIImage const&);

  void SetHidden(bool);

  // - MARK: 準備圖層內容

  // 呼叫 Delegate 進行繪畫，在呼叫子圖層進行繪畫，以重畫整個繪圖樹。
  void Draw(/* in */ HDC);

  // - MARK: 管理圖層位置與邊界

 public:
  UI::Rect frame_;

 private:
  mutable bool frame_change_flag_ = false;

  virtual auto FrameWillSet(const UI::Rect&) const -> const UI::Rect&;

  virtual void FrameDidSet();

  // - MARK: 更新圖層顯示

 public:
  // 設定圖層需要更新旗標為需要更新。
  void SetNeedDisplay();

  // 設定圖層的部分區域需要更新旗標為需要更新。
  void SetNeedDisplay(UI::Rect const&);

  // 假如需要更新畫面時立即開始執行更新畫面。
  void DisplayIfNeeded();

  // 返回是否需要更新畫面。
  bool NeedDisplay();

  // - MARK: 不同座標系間的轉換

  /**
   * 根據 `ConvertFlag`轉換為目標圖層座標係的座標或是從目標
   * 圖層座標系轉換為本圖層座標系。
   *
   * - Note: 目標必須屬於同一個 `UI::Window` 渲染樹內。
   *         如果目標 Layer 為 `nullptr` 則使用 RootLayer 為目標。
   *         如果目標圖層不為同一個 `UI::Window` 渲染樹則為未定義行為。
   **/
  UI::Point Convert(UI::Point, UI::Convert, std::shared_ptr<UI::Layer> const& = nullptr);

  /**
   * 根據 `ConvertFlag`轉換為目標圖層的範圍或是從目標圖層的
   * 範圍轉換為本圖層範圍。
   *
   * - Note: 目標必須屬於同一個 `UI::Window` 渲染樹內。
   *         如果目標 Layer 為 `nullptr` 則使用 RootLayer 為目標。
   *         如果目標圖層不為同一個 `UI::Window` 渲染樹則為未定義行為。
   **/
  UI::Rect Convert(UI::Rect, UI::Convert, std::shared_ptr<UI::Layer> const& = nullptr);

 private:
  std::shared_ptr<UI::Layer> superlayer_;
  std::vector<std::shared_ptr<UI::Layer>> sublayers_;
  UI::Color background_color_;
  bool shouldRasterize_;
  bool is_need_layout_;
  bool is_need_display_;
  bool is_hidden_;

  // - MARK: 圖層樹處理

  // 找到根 UI::Layer 指標。
  std::shared_ptr<UI::Layer> RootLayer();

  // 返回與根圖層間的所有圖層，不包括根圖層與本身。
  std::vector<std::shared_ptr<UI::Layer>> AllAncestorLayer();
};
}  // namespace UI

typedef std::shared_ptr<UI::Layer> UILayer;
