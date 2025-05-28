/*
+---------------------+
|        首部         | (Header)报文头，12字节，dns_header
+---------------------+
|        问题         | (Question)向域名服务器的查询请求，dns_question
+---------------------+
|        回答         | (Answer)对于查询问题的回复,dns_resource_record
+---------------------+
|       授权机构        | (Authority)指向授权域名服务器,dns_resource_record结构
+---------------------+
|       附加信息        | (Additional)附加信息,dns_resource_record结构
+---------------------+
*/
#pragma once
#include <stdio.h>         // 标准输入输出头文件
#include <stdlib.h>        // 标准库函数头文件，如malloc, free等
#include <string.h>        // 字符串操作函数头文件
#include <stdint.h>        // 定义了整型变量的精确宽度类型
#include <time.h>
#include <WinSock2.h>      // Windows Socket API的头文件
#include <ws2tcpip.h>      // 提供更多的socket函数和结构，如getaddrinfo等
#pragma comment(lib, "ws2_32.lib")  // 自动链接到ws2_32.lib库，该库是Windows下实现socket编程的库
#pragma warning(disable:4996)      // 禁用编译器警告4996



#define DNS_STRING_MAX_SIZE 4096  // DNS报文的最大长度 EDNS
#define DNS_RR_NAME_MAX_SIZE 255   //DNS域名的最大长度 
#define DNSDNS_RR_NAME_SINGLE_MAX_SIZE 63 //单个标签的最大字节数
#define DNS_QR_QUERY 0  // DNS查询，0表示查询
#define DNS_QR_ANSWER 1 // DNS响应标志，1表示响应
// DNS操作码，表示查询类型
#define DNS_OPCODE_QUERY 0  // 0表示标准查询
#define DNS_OPCODE_IQUERY 1 // 1表示反向查询
#define DNS_OPCODE_STATUS 2 // 2表示服务器状态请求
// DNS资源记录类型，表示不同类型的DNS记录
#define DNS_TYPE_A 1       // A记录，表示IPv4地址
#define DNS_TYPE_NS 2      // NS记录，表示权威名称服务器
#define DNS_TYPE_CNAME 5   // CNAME记录，表示规范名称
#define DNS_TYPE_SOA 6     // SOA记录，表示起始授权机构
#define DNS_TYPE_PTR 12    // PTR记录，表示指针记录
#define DNS_TYPE_HINFO 13  // HINFO记录，表示主机信息
#define DNS_TYPE_MINFO 14  // MINFO记录，表示邮件信息
#define DNS_TYPE_MX 15     // MX记录，表示邮件交换记录
#define DNS_TYPE_TXT 16    // TXT记录，表示文本记录
#define DNS_TYPE_AAAA 28   // AAAA记录，表示IPv6地址

#define DNS_CLASS_IN 1   // DNS类，表示地址类型，通常为1，表示因特网
 
// DNS响应代码，表示查询的返回状态
#define DNS_RCODE_OK 0  //0表示无错误
#define DNS_RCODE_NXDOMAIN 3 //3表示名字错误


/*DNS 首部 (Header)

首部是每个DNS报文都必须包含的部分，固定为12个字节，用于定义报文的类型、状态和控制参数。

| 字段 (Field) | 字节数 | 描述 |
| :--- | :--- | :--- |
| **会话标识 (ID)** | 2 | 一个16位的标识符，由客户端生成，服务器在响应时会原样返回，用于匹配查询和响应。 |
| **标志 (Flags)** | 2 | 一个16位的字段，包含多个子标志位，用于控制报文的行为。 |
| **问题数量 (QDCOUNT)** | 2 | "问题"部分中包含的查询记录数量，通常为1。 |
| **回答资源记录数 (ANCOUNT)** | 2 | "回答"部分中包含的资源记录（RR）数量。 |
| **授权资源记录数 (NSCOUNT)** | 2 | "授权机构"部分中包含的资源记录数量。 |
| **附加资源记录数 (ARCOUNT)** | 2 | "附加信息"部分中包含的资源记录数量。 |
*/
typedef struct dns_header
{   //会话标识 (ID)
    uint16_t ID;
    
    //标志 (Flags) 字段
     uint16_t QR:1; //查询/响应标志：0代表查询报文，1代表响应报文。
     uint16_t Opcode:4;//操作码：0代表标准查询，1代表反向查询，2代表服务器状态请求。
     uint16_t AA:1;//权威回答 (Authoritative Answer)：置1表示响应的DNS服务器是该域名的权威服务器。
     uint16_t TC:1;//截断 (Truncation)：置1表示响应因超过UDP报文512字节的限制而被截断。客户端应转用TCP重试。
     uint16_t RD:1;//期望递归 (Recursion Desired)：由客户端设置，置1表示期望服务器进行递归查询。
     uint16_t RA:1;//递归可用 (Recursion Available)：由服务器在响应中设置，置1表示该服务器支持递归查询。
     uint16_t Z:1;//保留字段：必须为0。
     uint16_t RCODE:1;//响应码 (Response Code)：0表示无错误；1表示格式错误；2表示服务器故障；3表示域名不存在（NXDOMAIN）。
     
     //计数字段
     uint16_t QDCOUNT;//问题数量  "问题"部分中包含的查询记录数量，通常为1。
     uint16_t ANCOUNT;//回答资源记录数 "回答"部分中包含的资源记录（RR）数量。
     uint16_t NSCOUNT;//授权资源记录数 "授权机构"部分中包含的资源记录数量。
     uint16_t ARCOUNT;//附加资源记录数 "附加信息"部分中包含的资源记录数量。 
}dns_header;



//问题部分 (Question Section)
typedef struct dns_question{
      //要查询的域名。域名的每个标签（如"www"）前有一个字节表示该标签的长度，整个域名以一个长度为0的字节结尾。
      //例如，www.example.com 会被编码为 3www7example3com0。
      char* QNAME;  //使用字符串指针表示

      uint16_t QTYPE:16; //指定查询的资源记录类型-->宏定义部分

      uint16_t QCLASS:16; //指定查询的类别。对于互联网应用，通常为 1 (IN)。

      struct dns_question* next; //指向下一个问题的指针，用于形成链表结构
}dns_question;

// RData 定义了DNS资源记录中资源数据(RDATA)部分的联合体结构
// 注意: 所有多字节整数在网络传输时均为大端字节序 (Big-Endian)
//       所有域名(char*)在实际报文中均采用标签压缩编码格式

union RData {
    // A 记录 (Type=1) - IPv4 地址
    struct {
        uint8_t address[4];         // 4 字节的 IPv4 地址
    } A_record;

    // AAAA 记录 (Type=28) - IPv6 地址
    struct {
        uint8_t address[16];        // 16 字节的 IPv6 地址
    } AAAA_record;

    // NS 记录 (Type=2) - 域名服务器
    struct {
        char* nsdname;              // 负责该域的权威域名服务器的域名
    } NS_record;

    // CNAME 记录 (Type=5) - 规范名称 (别名)
    struct {
        char* cname;                // 指向真实名称的别名域名
    } CNAME_record;

    // SOA 记录 (Type=6) - 权威记录的起始
    struct {
        char* mname;              // 主域名服务器的域名
        char* rname;              // 该区域管理员的邮箱地址域名 (第一个'.'通常代表'@')
        uint32_t serial;            // 区域文件的版本号，每次更新时递增
        uint32_t refresh;           // 从服务器应等待多长时间（秒）来检查更新
        uint32_t retry;             // 若刷新失败，从服务器应等待多长时间（秒）后重试
        uint32_t expire;            // 若持续无法连接主服务器，从服务器应在多长时间（秒）后停止对外提供服务
        uint32_t minimum;           // (也称 Negative Cache TTL) 用于设置域名不存在(NXDOMAIN)的否定缓存时间（秒）
    } SOA_record;

    // PTR 记录 (Type=12) - 指针记录 (反向DNS查询)
    struct {
        char* ptrdname;             // 指向一个域名的指针
    } PTR_record;

    // MX 记录 (Type=15) - 邮件交换
    struct {
        uint16_t preference;        // 优先级 (值越小，优先级越高)
        char* exchange;          // 邮件交换服务器的域名
    } MX_record;

    // TXT 记录 (Type=16) - 文本记录
    struct {
        // 一个或多个字符串，每个字符串前有一个字节表示其长度。
        // 例如 "v=spf1" "include:_spf.google.com"
        // RDLENGTH 会是所有部分的总长度。
        char* txt_data;             // 指向文本数据的指针
    } TXT_record;
};

// 定义DNS报文中的资源记录 (Resource Record)
typedef struct dns_RR {
    // --- Header Part (报文中的固定头部) ---

    char* name;                 // 资源记录关联的域名 (可变长度，可压缩)
    uint16_t type;              // 资源记录的类型码 (例如, 1 for A, 28 for AAAA)
    uint16_t rrClass;          // 资源记录的类别码 (通常为 1, 代表 IN for Internet)
    uint32_t ttl;               // 生存时间 (Time To Live)，单位为秒，表示该记录可被缓存的时间
    uint16_t rdLength;          // RDATA 字段的长度（单位：字节）

    // --- Data Part (实际数据部分) ---
    union RData rdata;          // 资源数据，其具体结构由 'type' 字段决定
    struct dns_RR* next;   // 指向下一个资源记录的指针，用于形成链表结构
} dns_rr;


// DNS 报文结构体
typedef struct dns_message {
    dns_header* header;
    dns_question* question;
    dns_rr* answer;
    dns_rr* authority;
    dns_rr* additional;
} dns_Message;