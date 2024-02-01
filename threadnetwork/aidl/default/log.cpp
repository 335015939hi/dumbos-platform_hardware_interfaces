/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "log.hpp"

#include <stdarg.h>

static android_LogPriority sLogLevel = ANDROID_LOG_INFO;

void setLogLevel(android_LogPriority loglevel) {
    sLogLevel = loglevel;
}

void platformLog(android_LogPriority logLevel, const char* format, va_list args) {
    if (logLevel >= sLogLevel) {
        __android_log_vprint(logLevel, LOG_TAG, format, args);
    }
}

void logCrit(const char* format, ...) {
    va_list args;

    va_start(args, format);
    platformLog(ANDROID_LOG_FATAL, format, args);
    va_end(args);
}

void logError(const char* format, ...) {
    va_list args;

    va_start(args, format);
    platformLog(ANDROID_LOG_ERROR, format, args);
    va_end(args);
}

void logWarn(const char* format, ...) {
    va_list args;

    va_start(args, format);
    platformLog(ANDROID_LOG_WARN, format, args);
    va_end(args);
}

void logInfo(const char* format, ...) {
    va_list args;

    va_start(args, format);
    platformLog(ANDROID_LOG_INFO, format, args);
    va_end(args);
}

void logDebg(const char* format, ...) {
    va_list args;

    va_start(args, format);
    platformLog(ANDROID_LOG_DEBUG, format, args);
    va_end(args);
}
