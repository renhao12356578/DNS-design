#include "dns_config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

// 默认配置值
static char* DEFAULT_HOST_PATH = "./hosts.txt";
static char* DEFAULT_LOG_PATH = "./log.txt";
static char* DEFAULT_DNS_SERVER = "10.3.9.5";

// 全局变量初始化
char* host_path = NULL;
char* LOG_PATH = NULL;
char* dnsServerAddress = NULL;
int log_mode = 0;      // 默认不开启日志记录
u_long socketMode = 0;    // 默认非阻塞模式

char IPAddr[DNS_RR_NAME_MAX_SIZE];
char domain[DNS_RR_NAME_MAX_SIZE];

// 打印帮助信息
void printHelpInfo() {
    printf(" ▄▄▄▄▄▄▄▄▄▄   ▄▄        ▄  ▄▄▄▄▄▄▄▄▄▄▄       ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄            ▄▄▄▄▄▄▄▄▄▄▄  ▄         ▄ \n");
    printf("▐░░░░░░░░░░▌ ▐░░▌      ▐░▌▐░░░░░░░░░░░▌     ▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░▌          ▐░░░░░░░░░░░▌▐░▌       ▐░▌\n");
    printf("▐░█▀▀▀▀▀▀▀█░▌▐░▌░▌     ▐░▌▐░█▀▀▀▀▀▀▀▀▀      ▐░█▀▀▀▀▀▀▀█░▌▐░█▀▀▀▀▀▀▀▀▀ ▐░▌          ▐░█▀▀▀▀▀▀▀█░▌▐░▌       ▐░▌\n");
    printf("▐░▌       ▐░▌▐░▌▐░▌    ▐░▌▐░▌               ▐░▌       ▐░▌▐░▌          ▐░▌          ▐░▌       ▐░▌▐░▌       ▐░▌\n");
    printf("▐░▌       ▐░▌▐░▌ ▐░▌   ▐░▌▐░█▄▄▄▄▄▄▄▄▄      ▐░█▄▄▄▄▄▄▄█░▌▐░█▄▄▄▄▄▄▄▄▄ ▐░▌          ▐░█▄▄▄▄▄▄▄█░▌▐░█▄▄▄▄▄▄▄█░▌\n");
    printf("▐░▌       ▐░▌▐░▌  ▐░▌  ▐░▌▐░░░░░░░░░░░▌     ▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░▌          ▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌\n");
    printf("▐░▌       ▐░▌▐░▌   ▐░▌ ▐░▌ ▀▀▀▀▀▀▀▀▀█░▌     ▐░█▀▀▀▀█░█▀▀ ▐░█▀▀▀▀▀▀▀▀▀ ▐░▌          ▐░█▀▀▀▀▀▀▀█░▌ ▀▀▀▀█░█▀▀▀▀ \n");
    printf("▐░▌       ▐░▌▐░▌    ▐░▌▐░▌          ▐░▌     ▐░▌     ▐░▌  ▐░▌          ▐░▌          ▐░▌       ▐░▌     ▐░▌     \n");
    printf("▐░█▄▄▄▄▄▄▄█░▌▐░▌     ▐░▐░▌ ▄▄▄▄▄▄▄▄▄█░▌     ▐░▌      ▐░▌ ▐░█▄▄▄▄▄▄▄▄▄ ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌       ▐░▌     ▐░▌     \n");
    printf("▐░░░░░░░░░░▌ ▐░▌      ▐░░▌▐░░░░░░░░░░░▌     ▐░▌       ▐░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░▌       ▐░▌     ▐░▌     \n");
    printf(" ▀▀▀▀▀▀▀▀▀▀   ▀        ▀▀  ▀▀▀▀▀▀▀▀▀▀▀       ▀         ▀  ▀▀▀▀▀▀▀▀▀▀▀  ▀▀▀▀▀▀▀▀▀▀▀  ▀         ▀       ▀      \n");
    printf("+------------------------------------------------------------------------------+\n");
    printf("|                      Welcome to use our DNS relay system!                    |\n");
    printf("|                       Designed by RenHao and PengQuanYu                      |\n");
    printf("|                                                                              |\n");
    printf("| Example: nslookup www.bupt.edu.cn 127.0.0.1                                  |\n");
    printf("|                                                                              |\n");
    printf("| Arguments:                                                                   |\n");
    printf("|   -l                         日志记录                                        |\n");
    printf("|   -d                         查看收发信息                                    |\n");
    printf("|   -dd                        开启调试模式                                    |\n");
    printf("|   -s [server_address]        设置远程DNS服务器地址                           |\n");
    printf("|   -m [mode]                  设置程序的运行模式:0/1  非阻塞/阻塞             |\n");
    printf("+------------------------------------------------------------------------------+\n");
}

// 初始化配置，包括命令行参数解析和各子系统初始化
void configInit(int argc, char* argv[]) {
    // 设置默认配置值
    host_path = strdup(DEFAULT_HOST_PATH);
    LOG_PATH = strdup(DEFAULT_LOG_PATH);
    dnsServerAddress = strdup(DEFAULT_DNS_SERVER);
    
    if (!host_path || !LOG_PATH || !dnsServerAddress) {
        fprintf(stderr, "内存分配失败\n");
        exit(EXIT_FAILURE);
    }

    // 获取程序运行参数
    getConfig(argc, argv);

    // 打印调试信息
    printf("配置初始化完成:\n");
    if(argc>=2) set_log_level(argv[1]);
    printf("  - Hosts path: %s\n", host_path);
    printf("  - Log path: %s\n", LOG_PATH);
    printf("  - DNS server: %s\n", dnsServerAddress);
    printf("  - Socket mode: %s\n", socketMode == 0 ? "非阻塞" : "阻塞");
    printf("  - Log mode: %s\n", log_mode ? "开启" : "关闭");

    // 初始化各子系统
    initSocket();
    initDnsResolver();
    cacheInit();
    initIdList();
    readHost();


    switch (socketMode) {
        case 0:
            setNonBlockingMode(); // 非阻塞模式
            break;
        case 1:
            setBlockingMode(); // 阻塞模式
            break;
        default:
            fprintf(stderr, "无效的socket模式: %d\n", socketMode);
            exit(EXIT_FAILURE);
    }

}

// 读取程序命令参数
void getConfig(int argc, char* argv[]) {
    printHelpInfo();

    for (int index = 1; index < argc; index++) {
        if (strcmp(argv[index], "-l") == 0) {
            log_mode = 1;    // 开启日志模式
        }
        else if (strcmp(argv[index], "-s") == 0 && index + 1 < argc) {
            // 设置远程DNS服务器地址，安全地替换现有地址
            free(dnsServerAddress);
            dnsServerAddress = strdup(argv[++index]);
            if (!dnsServerAddress) {
                log_message(ERROR, "远程DNS服务器地址内存分配失败\n");
                exit(EXIT_FAILURE);
            }
        }
        else if (strcmp(argv[index], "-m") == 0 && index + 1 < argc) {
            // 设置程序的运行模式
            socketMode = atoi(argv[++index]);
        }else if(strcmp(argv[index], "-p") == 0 && index + 1 < argc){
            free(host_path);
            host_path = strdup(argv[++index]);
            if (!host_path) {
                log_message(ERROR, "路径地址内存分配失败\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

// 读取hosts文件
void readHost() {
    FILE* hostFile = fopen(host_path, "r");

    if (!hostFile) {
        fprintf(stderr, "错误：无法打开hosts文件: %s\n", host_path);
        exit(EXIT_FAILURE);
    }

    getHostInfo(hostFile);
    fclose(hostFile);
}

// 解析hosts文件内容
void getHostInfo(FILE* file) {
    if (!file) return;

    int count = 0;
    int domain_count = 0;
    char ipBuffer[64];
    char domainBuffer[DNS_RR_NAME_MAX_SIZE];
    char currentDomain[DNS_RR_NAME_MAX_SIZE] = {0};
    uint8_t ipArray[MAX_IP_COUNT][4]; // 使用MAX_IP_COUNT存储多个IP
    uint8_t ip_count = 0;
    
    // 按行读取hosts文件
    while (fscanf(file, "%63s %s", ipBuffer, domainBuffer) == 2) {
        uint8_t ipBytes[4] = {0};
        
        // 转换IP地址字符串为字节数组
        if (TranIP(ipBytes, ipBuffer)) {
            // 检查是否与上一个域名相同
            if (strlen(currentDomain) > 0 && strcmp(currentDomain, domainBuffer) != 0) {
                // 域名不同，将上一个域名的所有IP插入到域名解析表
                uint32_t ttls[MAX_IP_COUNT];
                // 为本地hosts记录设置默认TTL (24小时)
                for (int i = 0; i < ip_count; i++) {
                    ttls[i] = 86400;
                }
                insertNode(ipArray, ttls, ip_count, currentDomain);
                domain_count++;
                // 重置IP计数
                ip_count = 0;
            }
            
            // 保存当前域名
            strncpy(currentDomain, domainBuffer, DNS_RR_NAME_MAX_SIZE - 1);
            currentDomain[DNS_RR_NAME_MAX_SIZE - 1] = '\0';
            
            // 添加当前IP地址到数组（如果还有空间）
            if (ip_count < MAX_IP_COUNT) {
                memcpy(ipArray[ip_count], ipBytes, 4);
                ip_count++;
                count++;
            } else {
                printf("警告:域名 %s 的IP地址数量超过最大限制 %d\n", domainBuffer, MAX_IP_COUNT);
            }
        } else {
            printf("警告:无效的IP地址格式: %s\n", ipBuffer);
        }
    }
    
    // 处理最后一个域名
    if (strlen(currentDomain) > 0 && ip_count > 0) {
        uint32_t ttls[MAX_IP_COUNT];
        // 为本地hosts记录设置默认TTL (24小时)
        for (int i = 0; i < ip_count; i++) {
            ttls[i] = 86400;
        }
        insertNode(ipArray, ttls, ip_count, currentDomain);
        domain_count++;
    }

    printf("已加载 %d 条域名地址信息，共 %d 个不同域名\n\n", count, domain_count);
}

// 记录DNS查询日志
void writeLog(char* domain, uint8_t* ip_addr) {
    if (!log_mode) return;  // 如果日志模式未开启，直接返回
    
    FILE* fp = fopen(LOG_PATH, "a");
    if (fp == NULL) {
        log_message(ERROR, "无法打开日志文件: %s\n", LOG_PATH);
        return;
    }

    // 获取并格式化当前时间
    time_t currentTime = time(NULL);
    struct tm* localTime = localtime(&currentTime);
    char timeString[32];
    strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", localTime);
    
    // 写入日志条目
    fprintf(fp, "%s  %s  ", timeString, domain ? domain : "unknown");

    // 根据IP地址是否存在写入不同的内容
    if (ip_addr) {
        fprintf(fp, "%d.%d.%d.%d\n", ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3]);
    } else {
        fprintf(fp, "Not found in local. Returned from remote DNS server.\n");
    }

    // 刷新缓冲区并关闭文件
    fflush(fp);
    fclose(fp);
}

// 清理配置资源
void cleanupConfig() {
    free(host_path);
    free(LOG_PATH);
    free(dnsServerAddress);
    
    host_path = NULL;
    LOG_PATH = NULL;
    dnsServerAddress = NULL;
}

// 将点分十进制IP地址转换为字节数组
// 返回1表示成功，0表示失败
int TranIP(uint8_t* dest, const char* src) {
    if (!dest || !src) return 0;
    
    int values[4];
    int count = sscanf(src, "%d.%d.%d.%d", &values[0], &values[1], &values[2], &values[3]);
    
    if (count != 4) return 0;
    
    // 验证每个值是否在有效范围内
    for (int i = 0; i < 4; i++) {
        if (values[i] < 0 || values[i] > 255) return 0;
        dest[i] = (uint8_t)values[i];
    }
    
    return 1;
}