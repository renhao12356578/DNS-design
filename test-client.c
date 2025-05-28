#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h> // 包含 inet_pton

#pragma comment(lib, "ws2_32.lib") // 告知编译器链接 ws2_32.lib 库

#define PORT 8080
#define SERVER_IP "127.0.0.1" // 服务器 IP 地址
#define BUFFER_SIZE 1024

int main() {
    WSADATA wsaData; // Winsock 初始化数据结构
    SOCKET client_socket; // 客户端 Socket
    struct sockaddr_in server_addr; // 服务器地址结构
    char buffer[BUFFER_SIZE] = {0};
    char input[BUFFER_SIZE] = {0};

    // 1. 初始化 Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed. Error Code : %d\n", WSAGetLastError());
        return 1;
    }
    printf("Winsock initialized.\n");

    // 2. 创建 Socket
    if ((client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
        fprintf(stderr, "Could not create socket. Error Code : %d\n", WSAGetLastError());
        WSACleanup(); // 创建失败，清理 Winsock
        return 1;
    }
    printf("Client socket created.\n");

    // 3. 配置服务器地址结构
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT); // 端口号，网络字节序

    // 4. 将字符串 IP 转换为网络字节序的二进制形式 (推荐使用 inet_pton)
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address/Address not supported. Error Code: %d\n", WSAGetLastError());
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }
    printf("Server address set to %s:%d\n", SERVER_IP, PORT);

    // 5. 连接到服务器
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "Connection failed with error code : %d\n", WSAGetLastError());
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }
    printf("Connected to server.\n");

    // 6. 循环发送和接收数据，直到用户选择退出
    printf("Chat started (type 'exit' to quit)...\n");
    
    while (1) {
        // 获取用户输入
        printf("Enter message: ");
        fgets(input, BUFFER_SIZE, stdin);
        
        // 移除换行符
        input[strcspn(input, "\n")] = 0;
        
        // 检查是否退出
        if (strcmp(input, "exit") == 0) {
            printf("Exiting...\n");
            break;
        }
        
        // 发送数据
        if (send(client_socket, input, strlen(input), 0) == SOCKET_ERROR) {
            fprintf(stderr, "Send failed with error code : %d\n", WSAGetLastError());
            break;
        }
        printf("Message sent: %s\n", input);
        
        // 接收服务器响应
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received == SOCKET_ERROR) {
            fprintf(stderr, "Recv failed with error code : %d\n", WSAGetLastError());
            break;
        } else if (bytes_received == 0) {
            printf("Server closed the connection.\n");
            break;
        } else {
            buffer[bytes_received] = '\0'; // 添加 null 终止符
            printf("Server reply: %s\n", buffer);
        }
    }

    // 8. 关闭 Socket
    closesocket(client_socket);
    printf("Client socket closed.\n");

    // 9. 清理 Winsock
    WSACleanup();
    printf("Winsock cleaned up.\n");

    return 0;
}
