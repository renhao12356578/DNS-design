#include "dns_table.h"

// --- 哈希表实现 ---

// 1. 哈希表中的单个条目结构体
typedef struct HashEntry {
    char domain[MAX_DOMAIN_LEN];
    uint8_t IP[4];
    struct HashEntry *next; // 指向下一个节点，用于处理哈希冲突
} HashEntry;

// 2. 哈希表的全局变量
static HashEntry **hashTable = NULL; // 哈希表本体（一个指针数组）
static int hashTableSize = 0;        // 哈希表的大小

// djb2哈希函数：一个简单且高效的字符串哈希算法
static unsigned long hash_function(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}

void insertNode(const uint8_t* IP, const char* domain) {
    if (!hashTable) return; // 检查是否已初始化

    unsigned long index = hash_function(domain) % hashTableSize;
    HashEntry* current = hashTable[index];

    // 检查域名是否已存在，如果存在则更新其IP
    while (current != NULL) {
        if (strcmp(current->domain, domain) == 0) {
            memcpy(current->IP, IP, 4);
            return;
        }
        current = current->next;
    }

    // 域名不存在，创建一个新条目
    HashEntry* newEntry = (HashEntry*)malloc(sizeof(HashEntry));
    if (!newEntry) {
        fprintf(stderr, "错误：为哈希表新条目分配内存失败。\n");
        return;
    }
    
    strncpy(newEntry->domain, domain, MAX_DOMAIN_LEN - 1);
    newEntry->domain[MAX_DOMAIN_LEN - 1] = '\0'; // 确保字符串正确结尾
    memcpy(newEntry->IP, IP, 4);

    // 将新条目插入到对应哈希桶链表的头部
    newEntry->next = hashTable[index];
    hashTable[index] = newEntry;
}

int queryNode(const char* domain, uint8_t* ip_addr) {
    if (!hashTable) return 0; // 检查是否已初始化

    unsigned long index = hash_function(domain) % hashTableSize;
    HashEntry* current = hashTable[index];

    // 遍历哈希桶的链表以查找域名
    while (current != NULL) {
        if (strcmp(current->domain, domain) == 0) {
            memcpy(ip_addr, current->IP, 4);
            return 1; // 找到
        }
        current = current->next;
    }
 
    return 0; // 未找到
}

// --- 初始化与销毁 ---

void initDnsResolver() {

    hashTableSize = TABLE_CAPACITY;
    // 使用calloc分配，它会自动将所有指针初始化为NULL
    hashTable = (HashEntry**)calloc(hashTableSize, sizeof(HashEntry*));
    if (!hashTable) {
        fprintf(stderr, "错误：为哈希表分配内存失败。\n");
        exit(1);
    }
}

void destroyDnsResolver(void) {
    if (!hashTable) return;

    // 遍历并释放哈希表中的所有条目
    for (int i = 0; i < hashTableSize; ++i) {
        HashEntry* current = hashTable[i];
        while (current != NULL) {
            HashEntry* temp = current;
            current = current->next;
            free(temp);
        }
    }
    // 释放哈希表本身
    free(hashTable);
    hashTable = NULL;
}

