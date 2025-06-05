#include "dns_resetid.h"

// 定义全局ID列表和我们的优化关键——游动指针
static uint16_t next_id = 0; // 这个静态变量会记住下一次搜索的起始位置

/**
 * @brief 初始化ID映射表。
 * 使用单个 memset 调用
 */
void initIdList() {
    // 使用单个memset将整个数组清零
    memset(IDList, 0, sizeof(IDList));
    next_id = 0; // 确保游动指针也重置
}

/**
 * @brief 为新的DNS请求分配一个ID，并返回其在表中的索引作为新的请求ID。
 * 采用环形指针法优化，避免了从头线性扫描的性能瓶颈。
 * @param userId 原始的DNS请求ID
 * @param clientAddress 原始客户端的地址信息
 * @return 成功时返回新的ID (0 到 MAX_ID_SIZE-1)，失败时返回UINT16_MAX。
 */
uint16_t resetId(uint16_t userId, struct sockaddr_in clientAddress) {
    time_t currentTime = time(NULL);

    // 从上次停止的地方开始，最多搜索一整圈
    for (int count = 0; count < MAX_ID_SIZE; ++count) {
        // 使用取模运算实现环形数组，保证索引在有效范围内
        uint16_t current_index = (next_id + count) % MAX_ID_SIZE;

        if (IDList[current_index].expireTime < currentTime) { // 检查ID是否已过期
            // 找到了一个可用的位置，填充数据
            IDList[current_index].userId = userId;
            IDList[current_index].clientAddress = clientAddress;
            IDList[current_index].expireTime = currentTime + ID_EXPIRE_TIME;

            // 更新游动指针，指向下一个位置，为下次分配做准备
            next_id = (current_index + 1) % MAX_ID_SIZE;

            // 直接返回找到的索引作为新ID
            return current_index;
        }
    }

    // 如果遍历了一整圈都没有找到可用的ID，说明表已满
    log_message(ERROR, "警告：ID映射表已满，无法分配新ID。\n");
    return MAX_ID_SIZE; // 使用标准常量表示失败
}