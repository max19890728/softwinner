/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "data/gui.h"

namespace UI {

struct Font {
  struct FontType {
   private:
    std::string key_;

   public:
    //
    static const char SXF[];

    // 使用原始位圖設備字體創建邏輯字體，即等寬位圖字體。
    static const char RAW[];

    // 使用 var-width 位圖設備字體創建邏輯字體。
    static const char VAR[];

    // 使用 qpf 設備字體創建邏輯字體。
    static const char QPF[];

    //
    static const char UPF[];

    // 使用位圖字體創建邏輯字體。
    static const char BMP[];

    // 使用可縮放的 TrueType 設備字體創建邏輯字體。
    static const char TTF[];

    // 使用可縮放的 Adob​​e Type1 設備字體創建邏輯字體。
    static const char T1F[];

    // 使用任何類型的設備字體創建邏輯字體。
    static const char ALL[];

    FontType() {}

    explicit FontType(const char key[]) : key_(key) {}

    inline auto operator=(const char new_key[]) -> const std::string& {
      return key_ = new_key;
    }

    inline operator const char*() { return key_.c_str(); }
  };

  // - MARK: 初始化器

  // 使用字型名稱與大小初始化一個 `UI::Font` 結構
  static inline auto init(std::string name, int size) -> UI::Font {
    return UI::Font{name, size, FontType::SXF};
  }

  explicit Font(std::string /* font name */, /* with */ int /* size */,
                const char* /* type */);

  ~Font();

  // - MARK: 創建系統字體

  static inline auto SystemFont(/* of */ int size) -> UI::Font {
    return UI::Font("arialuni", size, FontType::SXF);
  }

  // - MARK: 取得可用字型名稱

 public:
  static const std::vector<std::string> family_name_;

  std::string font_name_;

  // - MARK: 取得字型名稱

  // - MARK: 字體定義

  int point_size_;

  /* * * * * 其他成員 * * * * */

 public:
  FontType font_type_;

 private:
  PLOGFONT __font__;
  bool font_created_;

  /* * * * * 運算符重載 * * * * */

 public:
  inline operator const PLOGFONT() {
    if (!font_created_) {
      __font__ = CreateLogFont(
          font_type_, font_name_.c_str(), "UTF-8", FONT_WEIGHT_REGULAR,
          FONT_SLANT_ROMAN, FONT_SETWIDTH_NORMAL, FONT_OTHER_AUTOSCALE,
          FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE, point_size_, 0);
      font_created_ = __font__ != nullptr;
    }
    return __font__;
  }
};
}  // namespace UI
