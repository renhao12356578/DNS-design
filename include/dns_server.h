#pragma once
#define DNS_PORT 53
#define BUFFER_SIZE 1500  // DNS报文的最大尺寸

#include"dns_config.h"
#include"dns_convert.h"
#include"dns_struct.h"

u_long socketMode;           // 阻塞/非阻塞模式
int dnsSocket;               // 统一的DNS socket
struct sockaddr_in clientAddress;
struct sockaddr_in serverAddress;
int addressLength;

int clientPort;           // 客户端端口号
char* dnsServerAddress;   // 远程主机

int islisten;

void initSocket();
void closeSocketServer();
void setNonBlockingMode();
void setBlockingMode();
void receiveData();           // 合并后的数据接收函数
int isFromDnsServer(struct sockaddr_in* addr);   // 判断是否来自DNS服务器
void handleClientRequest(uint8_t* buffer, int msg_size, struct sockaddr_in* clientAddr);  // 处理客户端请求
void handleServerResponse(uint8_t* buffer, int msg_size);  // 处理服务器响应

