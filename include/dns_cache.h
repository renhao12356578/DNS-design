#pragma once
#include "dns_struct.h"
#include "dns_config.h"
#include <time.h>

#define MAX_DOMAIN_LEN 256        //最大域名长度
#define MAX_CACHE_SIZE 1024       // 默认缓存容量
#define HASH_TABLE_SIZE 2048      // 哈希表大小，通常是缓存容量的2倍
#define MAX_IP_COUNT 8            // 每个域名最多支持的IP地址数量

// 数据结构优化：使用双向链表节点
/**
 * @brief LRU缓存节点结构体
 * @param domain 域名字符串
 * @param IPs 多个IPv4地址数组
 * @param ip_count 有效IP地址的数量
 * @param ttl DNS记录的生存时间（秒）
 * @param insert_time 插入时间戳（用于计算是否过期）
 * @param prev 指向前一个节点的指针
 * @param next 指向下一个节点的指针
 */
typedef struct lruNode {   
    char domain[MAX_DOMAIN_LEN];  //域名
    uint8_t IPs[MAX_IP_COUNT][4]; //多个IPv4地址数组，最多MAX_IP_COUNT个
    uint32_t ttls[MAX_IP_COUNT];  //每个IP地址对应的TTL值（秒）
    uint8_t ip_count;             //当前记录的有效IP地址数量
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
 * @param ipv4s 输出参数，存储找到的多个IPv4地址
 * @param ip_count 输出参数，存储找到的IPv4地址数量
 * @param domain 要查询的域名
 * @return 成功返回1，未命中或已过期返回0
 */
int cacheGet(uint8_t ipv4s[][4], uint8_t* ip_count, char* domain);

/**
 * @brief 向缓存中插入新的域名及多个IP地址
 * @param ipv4s 要插入的多个IPv4地址数组
 * @param ttls 要插入的每个IP地址对应的TTL值数组，如果为NULL，则所有IP使用同一TTL
 * @param ip_count 要插入的IPv4地址数量
 * @param domain 要插入的域名
 * @param default_ttl 默认TTL值（秒），当ttls为NULL时使用
 */
void cachePut(uint8_t ipv4s[][4], const uint32_t ttls[], uint8_t ip_count, char* domain, uint32_t default_ttl);

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
