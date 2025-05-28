#include "dns_cache.h"
// --- 静态全局变量，用于存储缓存状态 ---

static int g_size;         // 当前缓存中的条目数
static int g_capacity;     // 缓存的总容量
static lruNode *g_head;    // 指向双向链表头部（最近使用的）
static lruNode *g_tail;    // 指向双向链表尾部（最久未使用的）
static hashNode **g_hash_table; // 哈希表本体


// --- 内部辅助函数 ---

// 简单的字符串哈希函数 (djb2)
static unsigned long hashFunction(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
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
        if (strcmp(current->domain, domain) == 0) {
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

// 查询操作
int cacheGet(uint8_t *ipv4, char* domain) {
    if (!g_hash_table) return 0; // 未初始化

    unsigned long index = hashFunction(domain) %HASH_TABLE_SIZE;
    hashNode* hash_node = g_hash_table[index];

    while(hash_node) {
        if (strcmp(hash_node->domain, domain) == 0) { // 命中
            lruNode* lru_node = hash_node->lru_node_ptr;
            memcpy(ipv4, lru_node->IP, 4);

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

// 插入操作
void cachePut(uint8_t ipv4[4], char* domain)
{
    if (!g_hash_table) return; // 未初始化
	
    unsigned long index = hashFunction(domain) %HASH_TABLE_SIZE;
    hashNode* hash_node = g_hash_table[index];

    // 1. 检查域名是否已存在于缓存中
    while(hash_node) {
        if (strcmp(hash_node->domain, domain) == 0) {
            // 已存在：更新IP值，并移动到链表头部
            lruNode* lru_node = hash_node->lru_node_ptr;
            memcpy(lru_node->IP, ipv4, 4);
            if (lru_node != g_head) {
                _unlinkNode(lru_node);
                _addNodeToFront(lru_node);
            }
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
    memcpy(new_lru_node->IP, ipv4, 4);

    // 插入到链表头部
    _addNodeToFront(new_lru_node);

    // 插入到哈希表
    hashNode* new_hash_node = (hashNode*)malloc(sizeof(hashNode));
    strncpy(new_hash_node->domain, domain, MAX_DOMAIN_LEN - 1);
    new_hash_node->domain[MAX_DOMAIN_LEN - 1] = '\0';
    new_hash_node->lru_node_ptr = new_lru_node;
    new_hash_node->next = g_hash_table[index]; // 插入到哈希桶链表的头部
    g_hash_table[index] = new_hash_node;

    g_size++;
}
