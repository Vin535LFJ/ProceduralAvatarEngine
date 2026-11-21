#pragma once

#include <cstdarg>
#include <cstdio>

namespace pae {

enum class LogLevel : int { Debug = 0, Info = 1, Warning = 2, Error = 3 };

void logMessage(LogLevel level, const char* tag, const char* format, ...);
void logMessageV(LogLevel level, const char* tag, const char* format, va_list args);

} // namespace pae

#define PAE_LOGD(tag, ...) ::pae::logMessage(::pae::LogLevel::Debug, tag, __VA_ARGS__)
#define PAE_LOGI(tag, ...) ::pae::logMessage(::pae::LogLevel::Info, tag, __VA_ARGS__)
#define PAE_LOGW(tag, ...) ::pae::logMessage(::pae::LogLevel::Warning, tag, __VA_ARGS__)
#define PAE_LOGE(tag, ...) ::pae::logMessage(::pae::LogLevel::Error, tag, __VA_ARGS__)
