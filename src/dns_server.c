#include"dns_server.h"

// 客户端端口和地址长度变量
int clientPort;
int addressLength;
// DNS服务器的IP地址 - defined in dns_config.c
extern char* dnsServerAddress;

void initSocket()
{
        addressLength = sizeof(struct sockaddr_in);
        // 初始化Winsock
        WORD wVersionRequested = MAKEWORD(2, 2);
        WSADATA wsaData;
        if (WSAStartup(wVersionRequested, &wsaData) != 0) {
            log_message(ERROR, "WSAStartup failed: %d\n", WSAGetLastError());
            exit(1);
        }

        // 创建单个UDP socket
        dnsSocket = socket(AF_INET, SOCK_DGRAM, 0);
        if (dnsSocket == INVALID_SOCKET) {
            log_message(ERROR, "Error opening DNS socket: %d\n", WSAGetLastError());
            WSACleanup();
            exit(1);
        }

        // 初始化本地地址结构（绑定到53端口）
        memset(&clientAddress, 0, sizeof(clientAddress));
        clientAddress.sin_family = AF_INET;
        clientAddress.sin_addr.s_addr = INADDR_ANY;
        clientAddress.sin_port = htons(DNS_PORT);

        // 初始化远程DNS服务器地址结构
        memset(&serverAddress, 0, sizeof(serverAddress));
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_addr.s_addr = inet_addr(dnsServerAddress);
        serverAddress.sin_port = htons(DNS_PORT);

        // 设置端口重用
        const int REUSEADDR_OPTION = 1;
        if (setsockopt(dnsSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&REUSEADDR_OPTION, sizeof(int)) == SOCKET_ERROR) {
            log_message(ERROR, "setsockopt(SO_REUSEADDR) failed: %d\n", WSAGetLastError());
            closesocket(dnsSocket);
            WSACleanup();
            exit(1);
        }

        // 绑定socket到本地53端口
        if (bind(dnsSocket, (struct sockaddr*)&clientAddress, sizeof(clientAddress)) == SOCKET_ERROR) {
            log_message(ERROR, "Bind failed with error: %d\n", WSAGetLastError());
            closesocket(dnsSocket);
            WSACleanup();
            exit(1);
        }

        // 打印服务器信息
        printf("DNS server: %s\n", dnsServerAddress);
        printf("Listening on port %d\n", DNS_PORT);
}

// 关闭套接字并清理Winsock
void closeSocketServer()
{
      closesocket(dnsSocket);
      WSACleanup();
}

// 非阻塞模式
void setNonBlockingMode()
{
    // 设置socket为非阻塞模式
    if (ioctlsocket(dnsSocket, FIONBIO, &socketMode) != 0) {
        log_message(ERROR, "Failed to set non-blocking socketMode: %d\n\n", WSAGetLastError());
        closesocket(dnsSocket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // 在非阻塞模式下，循环接收数据
    time_t last_cleanup = time(NULL);
    const int CLEANUP_INTERVAL = 60; // 每60秒清理一次过期缓存
    
    while (1) {
        receiveData();  // 统一的数据接收函数
        
        // 定期清理过期缓存
        time_t current_time = time(NULL);
        if (current_time - last_cleanup >= CLEANUP_INTERVAL) {
            int expired_count = cacheCleanExpired();
            if (expired_count > 0) {
                log_message(LOG_DEBUG,"Cache cleanup: removed %d expired entries\n", expired_count);
            }
            last_cleanup = current_time;
        }
        
        Sleep(1);
    }
}

// 使用WSAPoll进行阻塞模式下的事件监听
void setBlockingMode()
{
    // 使用WSAPoll进行阻塞模式下的事件监听
    struct pollfd fds[1];           // 只需要监听一个socket
    fds[0].fd = dnsSocket;          // 设置需要监听的套接字
    fds[0].events = POLLIN;         // 指定监听的事件类型为 "可读"

    time_t last_cleanup = time(NULL);
    const int CLEANUP_INTERVAL = 60; // 每60秒清理一次过期缓存

    // 循环等待直到有事件发生
    while (1) {
        //调用 WSAPoll 等待 socket 上的事件。timeout设为1000ms（1秒）以支持定期清理
        int ret = WSAPoll(fds, 1, 1000);
        if (ret == SOCKET_ERROR) {
            log_message(ERROR, "WSAPoll failed: %d\n", WSAGetLastError());
            closesocket(dnsSocket);
            WSACleanup();
            exit(EXIT_FAILURE);
        } else if (ret > 0) {
            if (fds[0].revents & POLLIN) {
                receiveData(); // 处理数据接收
            }
        }
        
        // 定期清理过期缓存（无论是否有网络事件）
        time_t current_time = time(NULL);
        if (current_time - last_cleanup >= CLEANUP_INTERVAL) {
            int expired_count = cacheCleanExpired();
            if (expired_count > 0) {
                log_message(LOG_DEBUG,"Cache cleanup: removed %d expired entries\n", expired_count);
            }
            last_cleanup = current_time;
        }
    }
}

// 判断地址是否为DNS服务器地址
int isFromDnsServer(struct sockaddr_in* addr) {
    return (addr->sin_addr.s_addr == serverAddress.sin_addr.s_addr && 
            addr->sin_port == serverAddress.sin_port);
}

// 统一的数据接收和处理函数
void receiveData() {
    uint8_t buffer[BUFFER_SIZE];
    struct sockaddr_in fromAddress;
    int fromAddressLength = sizeof(fromAddress);
    int msg_size = -1;

    // 接收数据并获取发送方地址
    msg_size = recvfrom(dnsSocket, buffer, sizeof(buffer), 0, 
                       (struct sockaddr*)&fromAddress, &fromAddressLength);
    
    if (msg_size <= 0) {
        return; // 没有数据或出错
    }

    log_message(LOG_INFO,"Received message from %s:%d", 
            inet_ntoa(fromAddress.sin_addr), ntohs(fromAddress.sin_port));

    // 根据发送方地址判断数据来源
    if (isFromDnsServer(&fromAddress)) {
        // 来自远程DNS服务器的响应
        handleServerResponse(buffer, msg_size);
    } else {
        // 来自客户端的请求
        handleClientRequest(buffer, msg_size, &fromAddress);
    }
}

// 处理客户端请求
void handleClientRequest(uint8_t* buffer, int msg_size, struct sockaddr_in* clientAddr) {
    uint8_t buffer_new[BUFFER_SIZE];  // 回复给客户端的报文
    dns_Message msg;                  // 报文结构体
    uint8_t ip_addr[4] = { 0 };       // 查询域名得到的IP地址
    int is_found = 0;                 // 是否查到

    log_message(LOG_INFO,"Processing client request");

    uint8_t* start = buffer;

    /* 解析客户端发来的DNS报文，将其保存到msg结构体内 */
    str_to_dnsstruct(&msg, buffer, start);
    log_message(LOG_INFO, "====================================================");
    log_message(LOG_INFO, "Received DNS message with [ID: %d], [Domain: %s]", msg.header->ID, msg.question->QNAME);
    printQuestionAndAnswer(msg);

    /* 从缓存查找 */
    is_found = cacheGet(ip_addr, msg.question->QNAME);
    if(is_found != 0 && msg.question->QTYPE == DNS_TYPE_A){
        log_message(LOG_DEBUG, "Cache hit for [Domain: %s] ,[IP: %d.%d.%d.%d]", msg.question->QNAME ,ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3]);
    }

    /* 若cache未查到，则从host文件查找 */
    if (is_found == 0 || msg.question->QTYPE != DNS_TYPE_A) {

        is_found = queryNode(msg.question->QNAME, ip_addr);
        if(is_found && msg.question->QTYPE == DNS_TYPE_A){
            log_message(LOG_DEBUG, "Found in local hosts file: [Domain: %s] -> [IP: %d.%d.%d.%d]", 
                        msg.question->QNAME, ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3]);
        }

        if(is_found&&ip_addr[0]==0&&ip_addr[1]==0&&ip_addr[2]==0&&ip_addr[3]==0) {
            log_message(LOG_INFO,"此网站已被拦截");

        }else{
        
            /*若不是ipv4报文则直接转发给远程服务器*/
            if(msg.question->QTYPE != DNS_TYPE_A) is_found = 0;


            /* 若未查到，则上交远程DNS服务器处理*/
            if (is_found == 0) {
                /* 给将要发给远程DNS服务器的包分配新ID */
                uint16_t newID = resetId(msg.header->ID, *clientAddr);
                if (newID == MAX_ID_SIZE) {
                    log_message(LOG_DEBUG,"ID list is full.");
                } else {
                    uint16_t newID_net = htons(newID); // 转换为网络字节序
                    memcpy(buffer, &newID_net, sizeof(uint16_t));
                    sendto(dnsSocket, buffer, msg_size, 0, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
                    log_message(LOG_DEBUG,"NewID: %d, OldID: %d", newID, msg.header->ID);
                    log_message(LOG_INFO,"Send to remote server [ID: %d], [Domain: %s]", newID,msg.question->QNAME);
                    log_message(LOG_INFO, "====================================================\n\n");
                }
                return;
            }
        }
    }

    uint8_t* end;
    end = dnsstruct_to_str(&msg, buffer_new, ip_addr);
    dns_Message new_msg;
    log_message(LOG_INFO,"Send to the client [ID: %d], [Domain: %s] with [IP: %d.%d.%d.%d]",msg.header->ID, msg.question->QNAME, ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3]);
    str_to_dnsstruct(&new_msg, buffer_new,buffer_new);
    printQuestionAndAnswer(new_msg);
    log_message(LOG_INFO, "====================================================\n\n");
    int len = end - buffer_new;
    
    /* 将DNS应答报文发回客户端 */
    sendto(dnsSocket, buffer_new, len, 0, (struct sockaddr*)clientAddr, sizeof(*clientAddr));

    if (log_mode == 1) {
        writeLog(msg.question->QNAME, ip_addr);
    }
}

// 处理服务器响应
void handleServerResponse(uint8_t* buffer, int msg_size) {
    dns_Message msg;

    /* 接受远程DNS服务器发来的DNS应答报文 */
    log_message(LOG_INFO,"Processing server response");
    log_message(LOG_INFO, "====================================================");
    str_to_dnsstruct(&msg, buffer, buffer);

    // 从DNS报文头部获取ID（已经是网络字节序）
    uint16_t receivedID_net;
    memcpy(&receivedID_net, buffer, sizeof(uint16_t));
    uint16_t receivedID = ntohs(receivedID_net); // 转换为主机字节序
    log_message(LOG_INFO, "Received from server [ID: %d], [Domain: %s]", receivedID, msg.question->QNAME);
    printQuestionAndAnswer(msg);
    
    /* ID转换 - 将新ID转换回原始ID */
    if (receivedID < MAX_ID_SIZE && IDList[receivedID].expireTime > 0) {
        uint16_t originalID = IDList[receivedID].userId;
        uint16_t originalID_net = htons(originalID);
        memcpy(buffer, &originalID_net, sizeof(uint16_t));  // 把待发回客户端的包ID改回原ID
        struct sockaddr_in originalClientAddress = IDList[receivedID].clientAddress;
        IDList[receivedID].expireTime = 0; // 清除ID映射
        sendto(dnsSocket, buffer, msg_size, 0, (struct sockaddr*)&originalClientAddress, sizeof(originalClientAddress));
        log_message(LOG_INFO, "Forwarded response to client [ID: %d], [Domain: %s]", originalID, msg.question->QNAME);
        // 处理从远程DNS服务器返回的IP地址记录
        uint8_t resolved_ip[4] = {0};
        // 从回答部分提取IP地址
        if (msg.answer && msg.answer->type == DNS_TYPE_A && msg.answer->rdLength == 4) {
            memcpy(resolved_ip, msg.answer->rdata.A_record.address, 4);
            
            // 将IP地址和TTL添加到缓存中
            if (msg.question && msg.question->QNAME) {
                uint32_t ttl = msg.answer->ttl; // TTL已经在readBits中转换为主机字节序
                cachePut(resolved_ip, msg.question->QNAME, ttl);
                log_message(LOG_DEBUG, "Added to cache: %s -> %d.%d.%d.%d (TTL: %u seconds)", 
                           msg.question->QNAME, 
                           resolved_ip[0], resolved_ip[1], resolved_ip[2], resolved_ip[3],
                           ttl);
            }
            
            // 记录到日志中，使用实际的IP地址
            if (log_mode == 1) {
                writeLog(msg.question->QNAME, resolved_ip);
            }
        } else {
            // 如果没有找到A记录或格式不正确，记录为未找到
            if (log_mode == 1) {
                writeLog(msg.question->QNAME, NULL);
            }
        }
    } else {
        log_message(LOG_ERROR,"Warning: Invalid or expired ID mapping: %d", receivedID);
    }
    log_message(LOG_INFO, "====================================================\n\n");
}