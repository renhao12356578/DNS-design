#pragma once
#include "dns_cache.h"
#include <winsock2.h> 
#include <ws2tcpip.h> 


#define MAX_ID_SIZE 2048   //ID映射表大小
#define ID_EXPIRE_TIME 4  // ID过期时间

typedef struct {
    uint16_t userId;           // 用户ID
    time_t expireTime;         // 过期时间
    struct sockaddr_in clientAddress; // 客户端地址
} ClientSession;

ClientSession IDList[MAX_ID_SIZE];  // 存储客户端会话信息的数组
void initIdList();
uint16_t resetId(uint16_t userId, struct sockaddr_in clientAddress);
