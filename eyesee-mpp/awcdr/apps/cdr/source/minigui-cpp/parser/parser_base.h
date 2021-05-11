/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: parser_base.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#pragma once

class Window;

class ParserBase {
 public:
  ParserBase();

  virtual ~ParserBase();

  void SetupUi(Window* owner);

 protected:
  virtual void start() = 0;

  Window* owner_;
};
