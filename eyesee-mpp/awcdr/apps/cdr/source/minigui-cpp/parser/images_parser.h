/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: images_parser.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#pragma once

#include <string>

#include "parser/xml_parser.h"
#include "type/types.h"

class ImageParser : public XmlParser {
 public:
  ImageParser();

  virtual ~ImageParser();

  std::string GetResourceFile();

  virtual std::string GetImagesFile();

 protected:
  virtual bool analysis();

  virtual void generate();

  StringMap file_map_;

 private:
  std::string prefix_;
};
