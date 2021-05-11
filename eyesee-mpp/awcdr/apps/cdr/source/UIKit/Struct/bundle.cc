/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "UIKit/Struct/bundle.h"

#include <string>

UI::Bundle const UI::Bundle::main = UI::Bundle{"/usr/share/minigui/"};

UI::Bundle::Bundle(std::string path) : path_(path) {}

std::string UI::Bundle::Path() { return path_.string(); }

std::string UI::Bundle::ResourcePath() { return path_.string() + "res/"; }

std::string UI::Bundle::Path(std::string name, std::string type,
                             std::string directory) {
  return Path() + (Path().back() == '/' ? "" : "/") +
         (directory.empty() ? "" : directory + "/") + name +
         (type.empty() ? "" : "." + type);
}
