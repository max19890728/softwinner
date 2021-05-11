/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: language.cpp
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#include "resource/language.h"
#include <errno.h>
#include <map>
#include <string>
#include <utility>

#undef LOG_TAG
#define LOG_TAG "Language"

Language* Language::instance_ = NULL;
pthread_mutex_t Language::mutex_ = PTHREAD_MUTEX_INITIALIZER;

Language* Language::get() {
  if (Language::instance_ == NULL) {
    pthread_mutex_lock(&Language::mutex_);
    if (Language::instance_ == NULL) {
      Language::instance_ = new Language();
    }
    pthread_mutex_unlock(&Language::mutex_);
  }
  return Language::instance_;
}

Language::Language() {
  mlanguage_conf.clear();
  mCurrentLangID = -1;
  mSupportLangNum = 0;
  mfont = CreateLogFont("sxf", "arialuni", "UTF-8", FONT_WEIGHT_REGULAR,
                        FONT_SLANT_ROMAN, FONT_SETWIDTH_NORMAL,
                        FONT_OTHER_AUTOSCALE, FONT_UNDERLINE_NONE,
                        FONT_STRUCKOUT_NONE, 26, 0);
  mfont_28 = CreateLogFont("sxf", "arialuni", "UTF-8", FONT_WEIGHT_REGULAR,
                           FONT_SLANT_ROMAN, FONT_SETWIDTH_NORMAL,
                           FONT_OTHER_AUTOSCALE, FONT_UNDERLINE_NONE,
                           FONT_STRUCKOUT_NONE, 28, 0);
  mfont_30 = CreateLogFont("sxf", "arialuni", "UTF-8", FONT_WEIGHT_REGULAR,
                           FONT_SLANT_ROMAN, FONT_SETWIDTH_NORMAL,
                           FONT_OTHER_AUTOSCALE, FONT_UNDERLINE_NONE,
                           FONT_STRUCKOUT_NONE, 30, 0);
  mfont_32 = CreateLogFont("sxf", "arialuni", "UTF-8", FONT_WEIGHT_REGULAR,
                           FONT_SLANT_ROMAN, FONT_SETWIDTH_NORMAL,
                           FONT_OTHER_AUTOSCALE, FONT_UNDERLINE_NONE,
                           FONT_STRUCKOUT_NONE, 32, 0);
  mfont_34 = CreateLogFont("sxf", "arialuni", "UTF-8", FONT_WEIGHT_REGULAR,
                           FONT_SLANT_ROMAN, FONT_SETWIDTH_NORMAL,
                           FONT_OTHER_AUTOSCALE, FONT_UNDERLINE_NONE,
                           FONT_STRUCKOUT_NONE, 34, 0);
  mfont_200 = CreateLogFont(FONT_TYPE_NAME_SCALE_TTF, "arialuni", "UTF-8",
                            FONT_WEIGHT_REGULAR, FONT_SLANT_ROMAN,
                            FONT_SETWIDTH_NORMAL, FONT_OTHER_AUTOSCALE,
                            FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE, 200, 0);
}

Language::~Language() {}

std::string Language::getLanguageFile() {
  std::map<int, std::string>::iterator it = mlanguage_conf.find(mCurrentLangID);
  if (it == mlanguage_conf.end()) return NULL;
  return it->second;
}

int Language::setLangID(int pLangID) {
  if (pLangID < 0 || pLangID >= mSupportLangNum) {
    db_error("this is a unsupported language, setlangID failed");
    return -1;
  }
  mCurrentLangID = pLangID;
  return 0;
}

int Language::initLanguage(StringIntMap pLangInfo) {
  StringIntMap::iterator iter;
  for (iter = pLangInfo.begin(); iter != pLangInfo.end(); iter++) {
    mSupportLangNum++;
    mlanguage_conf.insert(make_pair(iter->second, iter->first));
  }
  if (mSupportLangNum == 0) {
    db_error("initLanguage failed\n");
    return -1;
  }
  return 0;
}

LOGFONT* Language::GetFontBySize(int sz) {
  switch (sz) {
    case 28:
      return mfont_28;
    case 30:
      return mfont_30;
    case 32:
      return mfont_32;
    case 34:
      return mfont_34;
    case 200:
      return mfont_200;
    default:
      return mfont;
  }
}

LOGFONT* Language::GetFont() { return mfont; }
