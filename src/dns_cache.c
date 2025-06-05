#include "dns_cache.h"
#include "uthash.h"

// --- 静态全局变量，用于存储缓存状态 ---

static int g_size;         // 当前缓存中的条目数
static int g_capacity;     // 缓存的总容量
static lruNode *g_head;    // 指向双向链表头部（最近使用的）
static lruNode *g_tail;    // 指向双向链表尾部（最久未使用的）
static hashNode **g_hash_table; // 哈希表本体


// --- 内部辅助函数 ---

// 使用uthash提供的高质量哈希函数
// 默认使用Jenkins hash (HASH_JEN)，这是性能和分布都很好的哈希函数
static unsigned long hashFunction(const char *str) {
    unsigned hashv;
    HASH_JEN(str, strlen(str), hashv);
    return hashv;
}

// 检查指定的缓存节点是否已过期
// 只要有一个IP地址未过期，该节点就被视为未过期
int isExpired(lruNode* node) {
    if (!node || node->ip_count == 0) return 1;
    
    time_t current_time = time(NULL);
    for (int i = 0; i < node->ip_count; i++) {
        // 只要有一个IP未过期，就返回0表示未过期
        if ((current_time - node->insert_time) < node->ttls[i]) {
            return 0;
        }
    }
    // 所有IP都已过期
    return 1;
}

// 将节点从双向链表中解开
static void _unlinkNode(lruNode* node) {
    if (node->prev) {
        node->prev->next = node->next;
    } else { // node is head
        g_head = node->next;
    }

    if (node->next) {
        node->next->prev = node->prev;
    } else { // node is tail
        g_tail = node->prev;
    }
}

// 将节点添加到双向链表头部
static void _addNodeToFront(lruNode* node) {
    node->next = g_head;
    node->prev = NULL;
    if (g_head) {
        g_head->prev = node;
    }
    g_head = node;
    if (g_tail == NULL) {
        g_tail = node;
    }
}

// 从哈希表中移除一个条目
static void _removeFromHashTable(const char* domain) {
    unsigned long index = hashFunction(domain) % HASH_TABLE_SIZE;
    hashNode* current = g_hash_table[index];
    hashNode* prev = NULL;
    while(current) {
        if (strcmp(current->lru_node_ptr->domain, domain) == 0) {
            if (prev) {
                prev->next = current->next;
            } else {
                g_hash_table[index] = current->next;
            }
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

// --- 公开接口实现 ---
// 初始化
void cacheInit() {
	g_hash_table = (hashNode**)calloc(HASH_TABLE_SIZE, sizeof(hashNode*));
    if (!g_hash_table) {
        fprintf(stderr, "Error: Failed to allocate memory for hash table.\n");
        exit(1); // 如果内存分配失败，直接退出
    }
	g_size = 0;
	g_capacity = MAX_CACHE_SIZE;
	g_head = NULL;
	g_tail = NULL;
    printf("LRU Cache initialized with capacity %d.\n", g_capacity);
}

// 查询操作 - 支持多个IP地址返回
int cacheGet(uint8_t ipv4s[][4], uint8_t* ip_count, char* domain) {
    if (!g_hash_table) return 0; // 未初始化

    unsigned long index = hashFunction(domain) % HASH_TABLE_SIZE;
    hashNode* hash_node = g_hash_table[index];
    
    while(hash_node) {
        if (strcmp(hash_node->lru_node_ptr->domain, domain) == 0) { // 命中
            lruNode* lru_node = hash_node->lru_node_ptr;
            
            // 检查所有IP是否全部过期
            if (isExpired(lru_node)) {
                // 所有TTL已过期，从缓存中删除该条目
                log_message(LOG_DEBUG,"Cache entry for domain '%s' has all IPs expired", domain);
                _removeFromHashTable(domain);
                _unlinkNode(lru_node);
                free(lru_node);
                g_size--;
                return 0; // 返回0表示未命中（已过期）
            }
            
            // 至少有一个IP未过期，返回未过期的IP地址
            time_t current_time = time(NULL);
            *ip_count = 0;
            
            for (int i = 0; i < lru_node->ip_count; i++) {
                // 只返回未过期的IP地址
                if ((current_time - lru_node->insert_time) < lru_node->ttls[i]) {
                    memcpy(ipv4s[*ip_count], lru_node->IPs[i], 4);
                    (*ip_count)++;
                }
            }

            // LRU核心：将命中节点移动到链表头部
            if (lru_node != g_head) { // 如果不是头部节点才需要移动
                _unlinkNode(lru_node);
                _addNodeToFront(lru_node);
            }
            return 1; // 返回1表示命中
        }
        hash_node = hash_node->next;
    }
    
    return 0; // 返回0表示未命中
}

// 插入操作 - 支持多个IP地址和TTL插入
void cachePut(uint8_t ipv4s[][4], const uint32_t ttls[], uint8_t ip_count, char* domain, uint32_t default_ttl)
{
    if (!g_hash_table) return; // 未初始化
    
    // 确保ip_count不超过最大限制
    if (ip_count > MAX_IP_COUNT) {
        ip_count = MAX_IP_COUNT;
        log_message(LOG_DEBUG, "Warning: IP count exceeds maximum limit, truncated to %d", MAX_IP_COUNT);
    }
	
    unsigned long index = hashFunction(domain) % HASH_TABLE_SIZE;
    hashNode* hash_node = g_hash_table[index];

    // 1. 检查域名是否已存在于缓存中
    while(hash_node) {
        if (strcmp(hash_node->lru_node_ptr->domain, domain) == 0) {
            // 已存在：更新IP值、IP数量、每个IP的TTL和时间戳，并移动到链表头部
            lruNode* lru_node = hash_node->lru_node_ptr;
            lru_node->ip_count = ip_count;
            for (int i = 0; i < ip_count; i++) {
                memcpy(lru_node->IPs[i], ipv4s[i], 4);
                lru_node->ttls[i] = ttls ? ttls[i] : default_ttl;  // 为每个IP设置TTL
            }
            lru_node->insert_time = time(NULL);
            if (lru_node != g_head) {
                _unlinkNode(lru_node);
                _addNodeToFront(lru_node);
            }
            log_message(LOG_DEBUG,"Updated cache entry for domain '%s' with %d IPs", 
                       domain, ip_count);
            return;
        }
        hash_node = hash_node->next;
    }

    // 2. 是新条目，需要插入
    // 如果缓存已满，先淘汰最久未使用的条目（尾部节点）
    if (g_size >= g_capacity) {
        lruNode* tail_node = g_tail;
        _removeFromHashTable(tail_node->domain);
        _unlinkNode(tail_node);
        free(tail_node);
        g_size--;
    }

    // 3. 创建新的双向链表节点和哈希节点
    lruNode* new_lru_node = (lruNode*)malloc(sizeof(lruNode));
    strncpy(new_lru_node->domain, domain, MAX_DOMAIN_LEN -1);
    new_lru_node->domain[MAX_DOMAIN_LEN - 1] = '\0';
    
    // 复制所有的IP地址和TTL
    new_lru_node->ip_count = ip_count;
    for (int i = 0; i < ip_count; i++) {
        memcpy(new_lru_node->IPs[i], ipv4s[i], 4);
        new_lru_node->ttls[i] = ttls ? ttls[i] : default_ttl;  // 为每个IP设置TTL
    }
    
    new_lru_node->insert_time = time(NULL);

    // 插入到链表头部
    _addNodeToFront(new_lru_node);

    // 插入到哈希表
    hashNode* new_hash_node = (hashNode*)malloc(sizeof(hashNode));
    new_hash_node->lru_node_ptr = new_lru_node;
    new_hash_node->next = g_hash_table[index]; // 插入到哈希桶链表的头部
    g_hash_table[index] = new_hash_node;

    g_size++;
    log_message(LOG_DEBUG ,"Added new cache entry for domain '%s' with %d IPs", 
               domain, ip_count);
}

// 检查并清理所有过期的缓存条目
int cacheCleanExpired() {
    if (!g_hash_table) return 0;
    
    int expired_count = 0;
    lruNode* current = g_head;
    
    while (current) {
        lruNode* next = current->next; // 保存下一个节点，因为当前节点可能被删除
        
        if (isExpired(current)) {
            log_message(LOG_DEBUG ,"Cleaning expired cache entry for domain '%s'", current->domain);
            _removeFromHashTable(current->domain);
            _unlinkNode(current);
            free(current);
            g_size--;
            expired_count++;
        }
        
        current = next;
    }
    
    if (expired_count > 0) {
        log_message(LOG_DEBUG,"Cleaned %d expired cache entries\n", expired_count);
    }
    
    return expired_count;
}
