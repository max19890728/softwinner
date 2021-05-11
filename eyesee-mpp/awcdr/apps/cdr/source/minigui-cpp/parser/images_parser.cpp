/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: images_parser.cpp
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#include <parser/images_parser.h>

#include <utility>

#include "../application.h"
#include "debug/app_log.h"
#include "minigui-cpp/utils/utils.h"

#undef LOG_TAG
#define LOG_TAG "ImageParser"

#define QRC_BEGIN_FLAG ("<RCC>")
#define RESOURCE_BEGIN_FLAG ("<qresource prefix")
#define ALIAS_BEGIN_FLG ("<file alias")
#define FILE_BEGIN_FLG ("<file>")
#define RESOURCE_END_FLAG ("</qresource>")

enum { RES_NULL = 0, RES_START, RES_END, RES_FILE, RES_ALIAS };

ImageParser::ImageParser() { db_debug(" "); }

ImageParser::~ImageParser() {}

std::string ImageParser::GetResourceFile() { return GetImagesFile(); }

std::string ImageParser::GetImagesFile() {
  // string s("/usr/share/minigui/res/window/resource.qrc");
  // return s;
  return std::string();
}

static int ParseType(const std::string& line) {
  int pos = line.find(RESOURCE_BEGIN_FLAG);
  if (pos != -1) {
    return RES_START;
  }
  pos = line.find(RESOURCE_END_FLAG);
  if (pos != -1) {
    return RES_END;
  }
  pos = line.find(ALIAS_BEGIN_FLG);
  if (pos != -1) {
    return RES_ALIAS;
  }
  pos = line.find(FILE_BEGIN_FLG);
  if (pos != -1) {
    return RES_FILE;
  }
  return RES_NULL;
}

bool ImageParser::analysis() {
  StringVector::iterator it;
  int pos = lines_.begin()->find(QRC_BEGIN_FLAG);
  db_msg(" ");
  if (-1 == pos) {  // not a qrc file
    db_error("not a qrc file");
    return false;
  }
  db_msg(" ");
  it++;
  std::string result_full_alias;
  std::string result_alias;
  std::string result_full_file;
  std::string result_file;
  for (it = lines_.begin(); it != lines_.end(); it++) {
    switch (ParseType(*it)) {
      case RES_START:
        TrimString(it->c_str(), "\"", prefix_);
        prefix_ += "/";
        break;
      case RES_END:
        break;
      case RES_ALIAS:
        TrimString(it->c_str(), "\"", result_alias);
        // get the file's full path
        TrimString2(it->c_str(), ">", "<", result_file);
        result_full_file =
            prefix_ +
            result_file.substr(result_file.rfind("/", result_file.length()) +
                               1);
        file_map_.insert(make_pair(result_alias, result_full_file));
        db_msg("[RES_ALIAS]: alias %s file %s", result_alias.c_str(),
               result_full_file.c_str());
        break;
      case RES_FILE:
        // get the file's full path
        TrimString2(it->c_str(), ">", "<", result_file);
        result_full_file =
            prefix_ +
            result_file.substr(result_file.rfind("/", result_file.length()) +
                               1);
        result_full_alias = result_full_file.substr(
            result_full_file.rfind("/", result_full_file.length()) + 1);
        result_alias =
            result_full_alias.substr(0, result_full_alias.rfind("."));
        file_map_.insert(make_pair(result_alias, result_full_file));
        db_msg("[RES_FILE]: alias %s file %s", result_alias.c_str(),
               result_full_file.c_str());
        break;
    }
  }
  return true;
}

void ImageParser::generate() {}
