#include "output_level.h"
#include <stdarg.h>

// 设置日志等级函数
void set_log_level(const char* level_str) {
    if (strcmp(level_str, "-d") == 0) {
        current_level = LOG_INFO;
    }
    else if (strcmp(level_str, "-dd") == 0) {
        current_level = LOG_DEBUG;
    }
    else {
        current_level = LOG_ERROR;
    }
    printf("  - Output level: %s\n", current_level == LOG_DEBUG ? "DEBUG" :
           current_level == LOG_INFO ? "INFO" : "ERROR");
}

// 日志输出函数
void log_message(LogLevel level, const char* format, ...) {
    // 只输出小于等于当前设置等级的消息
    if (level <= current_level) {
        const char* level_str = "";
        switch (level) {
        case LOG_ERROR:   level_str = "ERROR"; break;
        case LOG_INFO:    level_str = "INFO"; break;
        case LOG_DEBUG:   level_str = "DEBUG"; break;
        }
        
        // 打印日志级别
        printf("[%s] ", level_str);
        
        // 处理可变参数
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        
        // 添加换行
        printf("\n");
    }
}