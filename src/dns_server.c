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
            fprintf(stderr, "WSAStartup failed: %d\n", WSAGetLastError());
            exit(1);
        }


        clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
        if (clientSocket == INVALID_SOCKET) {
            fprintf(stderr, "Error opening client socket: %d\n", WSAGetLastError());
            WSACleanup();
            exit(1);
        }

        serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
        if (serverSocket == INVALID_SOCKET) {
            fprintf(stderr, "Error opening server socket: %d\n", WSAGetLastError());
            closesocket(clientSocket);
            WSACleanup();
            exit(1);
        }

        // 初始化地址结构
        memset(&clientAddress, 0, sizeof(clientAddress));
        clientAddress.sin_family = AF_INET;
        clientAddress.sin_addr.s_addr = INADDR_ANY;
        clientAddress.sin_port = htons(DNS_PORT);

        memset(&serverAddress, 0, sizeof(serverAddress));
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_addr.s_addr = inet_addr(dnsServerAddress);
        serverAddress.sin_port = htons(DNS_PORT);

        // 设置端口重用
        const int REUSEADDR_OPTION = 1;
        if (setsockopt(clientSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&REUSEADDR_OPTION, sizeof(int)) == SOCKET_ERROR) {
            fprintf(stderr, "setsockopt(SO_REUSEADDR) failed: %d\n", WSAGetLastError());
            closesocket(clientSocket);
            closesocket(serverSocket);
            WSACleanup();
            exit(1);
        }

        // 绑定客户端socket
        if (bind(clientSocket, (struct sockaddr*)&clientAddress, sizeof(clientAddress)) == SOCKET_ERROR) {
            fprintf(stderr, "Bind failed with error: %d\n", WSAGetLastError());
            closesocket(clientSocket);
            closesocket(serverSocket);
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
      closesocket(clientSocket);
      closesocket(serverSocket);
      WSACleanup();
}

// 非阻塞模式
void setNonBlockingMode()
{
    // 尝试将服务器和客户端套接字设置为非阻塞模式
    if (ioctlsocket(serverSocket, FIONBIO, &socketMode) != 0 ||
        ioctlsocket(clientSocket, FIONBIO, &socketMode) != 0) {
        fprintf(stderr, "Failed to set non-blocking socketMode: %d\n\n", WSAGetLastError());
        closesocket(serverSocket);
        closesocket(clientSocket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // 在非阻塞模式下，循环接收来自客户端和服务器的数据
    while (1) {
        receiveClient();  // 接收来自客户端的数据
        receiveServer();  // 接收来自服务器的数据
        Sleep(1);
    }
}

// 使用WSAPoll进行阻塞模式下的事件监听
void setBlockingMode()
{
    // 使用WSAPoll进行阻塞模式下的事件监听
    struct pollfd fds[2];           // 用于存储需要监听的套接字和事件类型
    fds[0].fd = clientSocket;       // 设置需要监听的套接字
    fds[0].events = POLLIN;         // 指定监听的事件类型为 "可读"（有数据可读或连接已建立）
    fds[1].fd = serverSocket;
    fds[1].events = POLLIN;

    // 循环等待直到有事件发生
    while (1) {
        //调用 WSAPoll 等待 fds 中列出的套接字上的事件。-1 参数表示无限等待，直到有事件发生。
        int ret = WSAPoll(fds, 2, -1);
        if (ret == SOCKET_ERROR) {
            fprintf(stderr, "WSAPoll failed: %d\n", WSAGetLastError());
            closesocket(serverSocket);
            closesocket(clientSocket);
            WSACleanup();
            exit(EXIT_FAILURE);
        } else if (ret > 0) {
            if (fds[0].revents & POLLIN) {
                receiveClient(); // 处理客户端事件
            }
            if (fds[1].revents & POLLIN) {
                receiveServer(); // 处理服务器事件
            }
        }
    }
}

// 接收来自客户端的数据并进行处理
void receiveClient() {
    uint8_t buffer[BUFFER_SIZE];      // 接收的报文
    uint8_t buffer_new[BUFFER_SIZE];  // 回复给客户端的报文
    dns_Message msg;                  // 报文结构体
    uint8_t ip_addr[4] = { 0 };         // 查询域名得到的IP地址
    int msg_size = -1;                // 报文大小
    int is_found = 0;                 // 是否查到

    msg_size = recvfrom(clientSocket, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientAddress, &addressLength);
    if (debug_mode==1)
        printf("received for one message from client!\n\n");

    if (msg_size >= 0) {
        uint8_t* start = buffer;
        /* 解析客户端发来的DNS报文，将其保存到msg结构体内 */
        str_to_dnsstruct(&msg, buffer, start);

        /* 从缓存查找 */
        is_found = cacheGet(ip_addr,msg.question->QNAME);

        /* 若cache未查到，则从host文件查找 */
        if (is_found == 0) {
            if (debug_mode == 1)
                printf("Address not found in cache.  Try to find in local.\n\n");

            is_found = queryNode(msg.question->QNAME, ip_addr);

            /* 若未查到，则上交远程DNS服务器处理*/
            if (is_found == 0) {
                /* 给将要发给远程DNS服务器的包分配新ID */
                uint16_t newID = resetId(msg.header->ID, clientAddress);
                if (newID == MAX_ID_SIZE) {
                        printf("ID list is full.\n\n");
                } else {
                    islisten = 1;
                    uint16_t newID_net = htons(newID); // *** 转换为网络字节序 ***
                    memcpy(buffer, &newID_net, sizeof(uint16_t));
                    sendto(serverSocket, buffer, msg_size, 0, (struct sockaddr*)&serverAddress, addressLength);
                    if (debug_mode == 1) {
                        printf("Address not found in local.  Send to Remote Server.\n\n");
                        printf("NewID: %d, OldID: %d\n\n", newID, msg.header->ID);
                    }
                }
                return;
            }
        }
        uint8_t* end;
        end = dnsstruct_to_str(&msg, buffer_new, ip_addr);

        int len = end - buffer_new;

        /* 将DNS应答报文发回客户端 */
        sendto(clientSocket, buffer_new, len, 0, (struct sockaddr*)&clientAddress, addressLength);

        if (log_mode == 1) {
            writeLog(msg.question->QNAME, ip_addr);
        }
    }
}

// 接收来自远程DNS服务器的响应并转发给客户端
void receiveServer() {
    uint8_t buffer[BUFFER_SIZE];  // 接收的报文
    dns_Message msg;
    int msg_size = -1;  // 报文大小

    /* 接受远程DNS服务器发来的DNS应答报文 */
    if (islisten == 1) {
        msg_size = recvfrom(serverSocket, buffer, sizeof(buffer), 0, (struct sockaddr*)&serverAddress, &addressLength);
        if (debug_mode == 1)
            printf("received for one message from remote server!\n\n");
        
        if (msg_size > 0) {
            str_to_dnsstruct(&msg, buffer, buffer);
            
            // 从DNS报文头部获取ID（已经是网络字节序）
            uint16_t receivedID_net;
            memcpy(&receivedID_net, buffer, sizeof(uint16_t));
            uint16_t receivedID = ntohs(receivedID_net); // 转换为主机字节序
            
            if (debug_mode == 1) {
                printf("Received ID from server: %d\n", receivedID);
            }
            
            /* ID转换 - 将新ID转换回原始ID */
            if (receivedID < MAX_ID_SIZE && IDList[receivedID].expireTime > 0) {
                uint16_t originalID = IDList[receivedID].userId;
                uint16_t originalID_net = htons(originalID);
                memcpy(buffer, &originalID_net, sizeof(uint16_t));  // 把待发回客户端的包ID改回原ID

                struct sockaddr_in originalClientAddress = IDList[receivedID].clientAddress;
                IDList[receivedID].expireTime = 0; // 清除ID映射

                sendto(clientSocket, buffer, msg_size, 0, (struct sockaddr*)&originalClientAddress, addressLength);
                
                if (debug_mode == 1) {
                    printf("Forwarded response with original ID: %d\n\n", originalID);
                }

                // 处理从远程DNS服务器返回的IP地址记录
                if (log_mode == 1 || debug_mode == 1) {
                    uint8_t resolved_ip[4] = {0};
                    
                    // 从回答部分提取IP地址
                    if (msg.answer && msg.answer->type == DNS_TYPE_A && msg.answer->rdLength == 4) {
                        memcpy(resolved_ip, msg.answer->rdata.A_record.address, 4);
                        
                        // 将IP地址添加到缓存中
                        if (msg.question && msg.question->QNAME) {
                            cachePut(resolved_ip, msg.question->QNAME);
                            if (debug_mode == 1) {
                                printf("Added to cache: %s -> %d.%d.%d.%d\n", 
                                       msg.question->QNAME, 
                                       resolved_ip[0], resolved_ip[1], resolved_ip[2], resolved_ip[3]);
                            }
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
                }
                

            } else {
                if (debug_mode == 1) {
                    printf("Warning: Invalid or expired ID mapping: %d\n\n", receivedID);
                }
            }
        }
        islisten = 0;
    }
}