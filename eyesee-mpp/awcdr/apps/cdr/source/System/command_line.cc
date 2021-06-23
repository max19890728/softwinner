/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "System/command_line.h"

#include <algorithm>

#include "System/logger.h"
#include "common/extension/string.h"

void System::CommandLine::Send(std::string command,
                               std::function<void(std::string)> callback) {
  FILE* command_proc;
  auto logger = Logger("System::CommandLine");
  Log(LogLevel::Debug) << command << '\n';
  if ((command_proc = popen(command.c_str(), "r")) != nullptr) {
    char buffer[256];
    while (fgets(buffer, 256, command_proc) != nullptr) {
      auto message = std::string{buffer};
      RemoveNewLineIn(message);
      callback(message);
    }
    pclose(command_proc);
  } else {
    throw true;
  }
}
