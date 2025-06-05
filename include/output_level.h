#pragma once
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// 定义日志等级枚举
typedef enum {
    LOG_ERROR = 0,
    LOG_INFO,
    LOG_DEBUG,
} LogLevel;

// 当前日志等级（默认INFO）
static LogLevel current_level = LOG_ERROR;

void set_log_level(const char* level_str);

void log_message(LogLevel level, const char* format, ...);