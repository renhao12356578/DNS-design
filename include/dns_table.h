#pragma once
#include "dns_cache.h"
#include <stdint.h>
#include <time.h>

#define TABLE_CAPACITY  65536

/**
 * @brief 初始化DNS解析器系统
 *
 * @param table_capacity 哈希表的期望容量。较大的尺寸可以减少哈希冲突，提高性能。
 */
void initDnsResolver();

/**
 * @brief 销毁DNS解析器，释放所有已分配的内存，防止内存泄漏。
 */
void destroyDnsResolver(void);

/**
 * @brief 将一个"域名-多个IP"映射关系插入到哈希表中。
 * 如果域名已存在，其对应的IP地址将会被更新。
 *
 * @param IPs 多个IPv4地址数组，每个IPv4地址为长度为4的uint8_t数组
 * @param ttls 多个TTL值数组，每个TTL对应一个IP地址，如果为NULL则使用默认TTL
 * @param ip_count 要插入的IP地址数量
 * @param domain 要插入的域名字符串。
 */
void insertNode(const uint8_t IPs[][4], const uint32_t ttls[], uint8_t ip_count, const char* domain);

/**
 * @brief 在哈希表中查询一个域名。
 *
 * @param domain 要查询的域名字符串。
 * @param ip_addrs 二维数组，用于存储查询到的多个IP地址。
 * @param ip_count 输出参数，返回找到的IP地址数量。
 * @return 如果找到该域名（命中），返回1；否则返回0（未命中）。
 */
int queryNode(const char* domain, uint8_t ip_addrs[][4], uint8_t* ip_count);

