/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "System/logger.h"

#include <string>

#include "System/console_color.h"

std::map<LogLevel, const char*> System::Logger::level_tag_ = {
    {LogLevel::Debug, "\033[44;30m D \033[0m"},
    {LogLevel::Info, "\033[42;30m I \033[0m"},
    {LogLevel::Error, "\033[41;30m E \033[0m"}};

std::map<LogLevel, const char*> System::Logger::level_color_ = {
    {LogLevel::Debug, System::Console::Format::Light::blue},
    {LogLevel::Info, System::Console::Format::Light::green},
    {LogLevel::Error, System::Console::Format::Light::red}};
