/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: main_parser.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#pragma once

#define NDEBUG

#include "parser/xml_parser.h"
#include "type/types.h"

class MainParser : public XmlParser {
 public:
  MainParser();

  virtual ~MainParser();

  virtual std::string GetResourceFile();

 protected:
  virtual bool analysis();

  virtual void generate();

  void ParseWidget(std::string line);

  int GetParentIndex();

  int GetType(std::string &line);

  void GetText(StringVector::iterator &it, ObjectInfo &info);

  void GetGeometry(StringVector::iterator &it, ObjectInfo &info);

  void ParseProperty(StringVector::iterator &it);

 private:
  StringVector keywords_;
  StringMap ui_class_map_;
};
