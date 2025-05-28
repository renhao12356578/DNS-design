#pragma once
#define DNS_PORT 53
#define BUFFER_SIZE 1500  // DNS报文的最大尺寸

#include"dns_config.h"
#include"dns_convert.h"
#include"dns_struct.h"

int socketMode;           // 阻塞/非阻塞模式
int clientSocket;         // 接管主机DNS请求的socket
int serverSocket;         // 转发请求给远程DNS服务器的socket
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
void receiveClient();
void receiveServer();

