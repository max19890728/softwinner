/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <log/std_log.h>

#include <array>
#include <iomanip>
#include <ios>
#include <iostream>
#include <map>

#include "System/console_color.h"
#include "System/custom_debug_string_convertible.h"

#define Log(level)                                                            \
  logger(level) << System::Logger::level_color_[level] << "<" << __FUNCTION__ \
                << ":" << __LINE__ << "> "                                    \
                << System::Console::Format::default_

enum class LogLevel { Debug, Info, Error, All };

namespace System {

struct Logger {
 public:
  static std::map<LogLevel, const char*> level_tag_;

  static std::map<LogLevel, const char*> level_color_;

  // - MARK: 初始化器
 public:
  explicit inline Logger(std::string name = "Unknown",
                         std::ostream& output = std::cout)
      : level_(LogLevel::Debug), name_(name), output_(output) {}

  explicit inline Logger(LogLevel level, std::string name = "Unknown",
                         std::ostream& output = std::cout)
      : level_(level), name_(name), output_(output) {}

  // - MARK: 運算符重載

  inline auto& operator()(LogLevel log_level = LogLevel::Debug) {
    level_ = log_level;
    if (level_ <= Logger::GetLogLevel()) {
      output_ << std::left << std::setw(20) << name_;
      output_ << Logger::level_tag_[log_level];
      output_ << " ";
    }
    return *this;
  }

  template <typename T>
  friend inline auto operator<<(Logger& logger, const T& var) -> Logger& {
    if (logger.level_ <= Logger::GetLogLevel()) {
      logger.output_ << var;
    }
    return logger;
  }

  /* * * * * 其他成員 * * * * */
 public:
  static inline auto GetLogLevel() -> LogLevel& {
    static LogLevel log_level_internal = LogLevel::All;
    return log_level_internal;
  }

  static inline void SetLogLevel(LogLevel new_level) {
    GetLogLevel() = new_level;
  }

  inline void SetLoggerName(std::string new_name) {
    name_ = new_name;
  }

 private:
  LogLevel level_;
  std::string name_;
  std::ostream& output_;
};
}  // namespace System
