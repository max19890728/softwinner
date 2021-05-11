/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: resource_manager.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#pragma once

#include <string>
#include "debug/app_log.h"
#include "parser/images_parser.h"
#include "parser/string_parser.h"
#include "resource/language.h"
#include "type/types.h"

class Language;

class R : public ImageParser, StringParser {
 public:
  R();
  ~R();
  static R* get();
  std::string GetImagePath(const char* alias);
  void LoadImagesPath();
  void LoadImages(std::string path);
  std::string GetImagesFile();

  /* load the strings what the TextView displays */
  void LoadStrings(StringMap& text_map);
  LOGFONT* GetFont();
  LOGFONT* GetFontBySize(int sz);
  std::string GetLangFile();
  int SetLangID(int pLangID);
  int InitLanguage(StringIntMap pLangInfo);
  void GetString(std::string item_name, std::string& result);
  void GetStringArray(std::string array_name, StringVector& result);
  // int GetCurrentLanglD();
 private:
  static pthread_mutex_t mutex_;
  static R* instance_;
  std::string strings_file_;
  std::string images_file_;
  Language* language_;
};
