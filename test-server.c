#define _WINSOCK_DEPRECATED_NO_WARNINGS // 允许使用旧的 inet_ntoa 函数，现代代码推荐使用 inet_ntop
#define _CRT_SECURE_NO_WARNINGS // 允许使用 sprintf 等函数而不显示安全警告

#include <stdio.h>
#include <winsock2.h> // 包含 Winsock 函数和结构
#include <ws2tcpip.h> // 包含 inet_pton, inet_ntop 等函数

#pragma comment(lib, "ws2_32.lib") // 告知编译器链接 ws2_32.lib 库

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    WSADATA wsaData; // Winsock 初始化数据结构
    SOCKET server_fd, new_socket; // 服务器监听 Socket 和新连接的 Socket
    struct sockaddr_in server_addr, client_addr; // 服务器和客户端地址结构
    int client_addr_len = sizeof(client_addr); // 客户端地址结构大小 (注意这里是 int*)
    char buffer[BUFFER_SIZE] = {0};
    int bytes_received; // 接收到的字节数

    // 1. 初始化 Winsock
    // 请求 2.2 版本的 Winsock API
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed. Error Code : %d\n", WSAGetLastError());
        return 1;
    }
    printf("Winsock initialized.\n");

    // 2. 创建 Socket
    // AF_INET: IPv4 地址族
    // SOCK_STREAM: 流式套接字 (TCP)
    // IPPROTO_TCP: TCP 协议 (通常也可以用 0 让系统自动选择)
    if ((server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
        fprintf(stderr, "Could not create socket. Error Code : %d\n", WSAGetLastError());
        WSACleanup(); // 创建失败，清理 Winsock
        return 1;
    }
    printf("Socket created successfully.\n");

    // 3. 配置服务器地址结构
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // 监听所有可用接口
    server_addr.sin_port = htons(PORT);      // 端口号，网络字节序

    // Optional: Set socket option to reuse address
    // 可选：设置 Socket 选项允许地址重用
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) == SOCKET_ERROR) {
        fprintf(stderr, "setsockopt failed with error: %d\n", WSAGetLastError());
        // Non-fatal, continue
    }

    // 4. 绑定 Socket
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "Bind failed with error code : %d\n", WSAGetLastError());
        closesocket(server_fd); // 绑定失败，关闭 Socket
        WSACleanup();
        return 1;
    }
    printf("Socket bound to port %d.\n", PORT);

    // 5. 监听连接
    if (listen(server_fd, 10) == SOCKET_ERROR) { // 10: 连接请求队列长度
        fprintf(stderr, "Listen failed with error code : %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }
    printf("Server listening on port %d...\n", PORT);

    // 6. 循环接受客户端连接并处理
    while (1) {
        printf("Waiting for incoming connections...\n");
        // 接受客户端连接 (阻塞)
        if ((new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len)) == INVALID_SOCKET) {
            fprintf(stderr, "Accept failed with error code : %d\n", WSAGetLastError());
            // Continue accepting other connections
            continue;
        }
        printf("Accepted connection from %s:%d\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port)); // 注意 inet_ntoa 返回静态缓冲区

        // 7. 持续处理客户端数据，直到客户端断开连接
        printf("Start receiving client messages...\n");
        
        while (1) {
            // 清空缓冲区
            memset(buffer, 0, BUFFER_SIZE);
            
            // 接收数据
            bytes_received = recv(new_socket, buffer, BUFFER_SIZE - 1, 0); // -1 留给 null 终止符
            
            if (bytes_received == SOCKET_ERROR) {
                int error_code = WSAGetLastError();
                if (error_code == WSAECONNRESET) { // 客户端突然关闭连接
                    printf("Client disconnected unexpectedly.\n");
                } else {
                    fprintf(stderr, "Receive failed with error code: %d\n", error_code);
                }
                break;
            } else if (bytes_received == 0) { // 连接已正常关闭
                printf("Client disconnected gracefully.\n");
                break;
            } else {
                buffer[bytes_received] = '\0'; // 添加 null 终止符
                printf("Received from client: %s\n", buffer);

                // 发送确认消息
                char response[BUFFER_SIZE];
                sprintf_s(response, BUFFER_SIZE, "Server received: %s", buffer);
                if (send(new_socket, response, strlen(response), 0) == SOCKET_ERROR) {
                    fprintf(stderr, "Send failed with error code: %d\n", WSAGetLastError());
                    break;
                }
            }
        }

        // 8. 关闭客户端连接 Socket
        closesocket(new_socket);
        printf("Client connection closed.\n");
    }

    // 9. 关闭监听 Socket (服务器不停止通常不会执行到这里)
    closesocket(server_fd);

    // 10. 清理 Winsock
    WSACleanup();
    printf("Server shutting down.\n");

    return 0;
}
