/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: language.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#pragma once

#include <map>
#include <string>
#include "data/gui.h"
#include "debug/app_log.h"
#include "type/types.h"

typedef struct LanguageInfo_ {
  int id;
  std::string path;
} LanguageInfo;

class Language {
 public:
  Language();
  ~Language();
  static Language *get();
  LOGFONT *GetFont();
  LOGFONT *GetFontBySize(int sz);
  std::string getLanguageFile();
  int setLangID(int pLangID);
  int initLanguage(StringIntMap pLangInfo);

 private:
  LOGFONT *mfont;
  std::map<int, std::string> mlanguage_conf;
  int mCurrentLangID;
  static pthread_mutex_t mutex_;
  static Language *instance_;
  int mSupportLangNum;
};
