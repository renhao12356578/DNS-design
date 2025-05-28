#pragma once
#include "dns_struct.h"
#include "dns_config.h"

#define MAX_DOMAIN_LEN 256
#define MAX_CACHE_SIZE 1024 // 默认缓存容量
#define HASH_TABLE_SIZE 2048 // 哈希表大小，通常是缓存容量的2倍

// 数据结构优化：使用双向链表节点
typedef struct lruNode {   
    char domain[MAX_DOMAIN_LEN];
    uint8_t IP[4];
    struct lruNode *prev;
    struct lruNode *next;
}lruNode;	

// 哈希表节点，用于处理哈希冲突（链地址法）
typedef struct hashNode {
    char domain[MAX_DOMAIN_LEN];
    lruNode *lru_node_ptr; // 指向双向链表中的节点
    struct hashNode *next;
} hashNode;



//接口优化：提供创建和销毁函数
//初始化cache
void cacheInit();
// 在cache中查询某个域名
int cacheGet(uint8_t* ipv4, char* domain);

// 插入新的域名及ip
void cachePut(uint8_t ipv4[4], char* domain);

//删除最老的域名及ip
void cacheDelete();
