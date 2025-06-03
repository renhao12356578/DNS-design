#include "output_level.h"

// 设置日志等级函数
void set_log_level(const char* level_str) {
    if (strcmp(level_str, "-d") == 0) {
        current_level = INFO;
    }
    else if (strcmp(level_str, "-dd") == 0) {
        current_level = DEBUG;
    }
    else {
        current_level = ERROR;
    }
    printf("  - Output level: %s\n", current_level == DEBUG ? "DEBUG" :
           current_level == INFO ? "INFO" : "ERROR");
}

// 日志输出函数
void log_message(LogLevel level, const char* message) {
    // 只输出小于等于当前设置等级的消息
    if (level <= current_level) {
        const char* level_str = "";
        switch (level) {
        case ERROR:   level_str = "ERROR"; break;
        case INFO: level_str = "INFO"; break;
        case DEBUG:    level_str = "DEBUG"; break;
        }
        printf("[%s] %s\n", level_str, message);
    }
}