#pragma once
#include "dns_struct.h"
#include "dns_config.h"
#include <time.h>

#define MAX_DOMAIN_LEN 256        //最大域名长度
#define MAX_CACHE_SIZE 1024       // 默认缓存容量
#define HASH_TABLE_SIZE 2048      // 哈希表大小，通常是缓存容量的2倍

// 数据结构优化：使用双向链表节点
/**
 * @brief LRU缓存节点结构体
 * @param domain 域名字符串
 * @param IP IPv4地址
 * @param ttl DNS记录的生存时间（秒）
 * @param insert_time 插入时间戳（用于计算是否过期）
 * @param prev 指向前一个节点的指针
 * @param next 指向下一个节点的指针
 */
typedef struct lruNode {   
    char domain[MAX_DOMAIN_LEN];  //域名
    uint8_t IP[4];                //IPv4地址
    uint32_t ttl;                 //DNS记录的TTL值（秒）
    time_t insert_time;           //记录插入时间戳
    struct lruNode *prev;         
    struct lruNode *next;         
} lruNode;

// 哈希表节点，用于处理哈希冲突（链地址法）
/**
 * @brief 哈希表节点结构体
 * @param lru_node_ptr 指向对应的LRU缓存节点
 * @param next 指向同一哈希桶中的下一个节点
 */
typedef struct hashNode {
    lruNode *lru_node_ptr;        //指向哈希桶中双向链中的节点
    struct hashNode *next;        //指向同一哈希桶中的下一个节点
} hashNode;



//接口优化：提供创建和销毁函数
/**
 * @brief 初始化LRU缓存
 */
void cacheInit();

/**
 * @brief 在cache中查询某个域名
 * @param ipv4 输出参数，存储找到的IPv4地址
 * @param domain 要查询的域名
 * @return 成功返回1，未命中或已过期返回0
 */
int cacheGet(uint8_t* ipv4, char* domain);

/**
 * @brief 向缓存中插入新的域名及IP地址
 * @param ipv4 要插入的IPv4地址
 * @param domain 要插入的域名
 * @param ttl DNS记录的TTL值（秒）
 */
void cachePut(uint8_t ipv4[4], char* domain, uint32_t ttl);

/**
 * @brief 删除最老的域名及IP地址
 */
void cacheDelete();

/**
 * @brief 检查并清理所有过期的缓存条目
 * @return 清理的过期条目数量
 */
int cacheCleanExpired();

/**
 * @brief 检查指定的缓存节点是否已过期
 * @param node 要检查的缓存节点
 * @return 1表示已过期，0表示未过期
 */
int isExpired(lruNode* node);
