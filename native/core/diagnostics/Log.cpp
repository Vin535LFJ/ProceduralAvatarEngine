#include "Log.h"

#ifdef PAE_ANDROID
#include <android/log.h>
#endif

namespace pae {

void logMessageV(LogLevel level, const char* tag, const char* format, va_list args) {
#ifdef PAE_ANDROID
    int priority = ANDROID_LOG_INFO;
    switch (level) {
        case LogLevel::Debug: priority = ANDROID_LOG_DEBUG; break;
        case LogLevel::Info: priority = ANDROID_LOG_INFO; break;
        case LogLevel::Warning: priority = ANDROID_LOG_WARN; break;
        case LogLevel::Error: priority = ANDROID_LOG_ERROR; break;
    }
    __android_log_vprint(priority, tag ? tag : "PAE", format ? format : "", args);
#else
    const char* prefix = "I";
    switch (level) {
        case LogLevel::Debug: prefix = "D"; break;
        case LogLevel::Info: prefix = "I"; break;
        case LogLevel::Warning: prefix = "W"; break;
        case LogLevel::Error: prefix = "E"; break;
    }
    std::fprintf(stderr, "[%s/%s] ", prefix, tag ? tag : "PAE");
    std::vfprintf(stderr, format ? format : "", args);
    std::fprintf(stderr, "\n");
#endif
}

void logMessage(LogLevel level, const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    logMessageV(level, tag, format, args);
    va_end(args);
}

} // namespace pae
