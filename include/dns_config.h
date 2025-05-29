#pragma once
#include "dns_struct.h"
#include "dns_server.h"
#include "dns_resetid.h"  // 包含IDList的定义
#include "dns_table.h"

// 调试和日志模式
extern int debug_mode;
extern int log_mode;
extern u_long socketMode;
extern int islisten;

// 路径配置
extern char* host_path;  
extern char* LOG_PATH;
extern char* dnsServerAddress;

// 临时缓冲区
extern char IPAddr[DNS_RR_NAME_MAX_SIZE];
extern char domain[DNS_RR_NAME_MAX_SIZE];



// 配置初始化与清理
void configInit(int argc, char* argv[]);
void cleanupConfig();

// 命令行参数解析
void getConfig(int argc, char* argv[]);
void printHelpInfo();

// 日志记录
void writeLog(char* domain, uint8_t* ip_addr);

// Hosts文件处理
void readHost();
void getHostInfo(FILE* ptr);

// IP地址转换
int TranIP(uint8_t* dest, const char* src);






