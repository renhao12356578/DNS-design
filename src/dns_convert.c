//本文件用于实现DNS报文结构体与字串之间的转换等报文操作
#include "dns_convert.h"
#include "dns_config.h"
#include "dns_mes_print.h"

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// 从缓冲区中获取指定数量的位（8, 16, 32）并返回相应的值
size_t readBits(uint8_t** buffer, int bits) {
    switch (bits) {
        case 8: {
            uint8_t val;
            memcpy(&val, *buffer, 1);
            *buffer += 1;
            return val;
        }
        case 16: {
            uint16_t val;
            memcpy(&val, *buffer, 2);
            *buffer += 2;
            return ntohs(val); // 网络字节序转换为主机字节序
        }
        case 32: {
            uint32_t val;
            memcpy(&val, *buffer, 4);
            *buffer += 4;
            return ntohl(val); // 网络字节序转换为主机字节序
        }
        default:
            return 0; // 不支持的位数
    }
}

// 解析收到的DNS报文并存储到msg结构体中
void str_to_dnsstruct(dns_Message* pmsg, uint8_t* buffer, uint8_t* start) {
    if (!pmsg || !buffer || !start) {
        return; // 参数无效
    }

    // 开辟空间
    pmsg->header = malloc(sizeof(dns_header));
    if (!pmsg->header) return; // 内存分配失败

    pmsg->question = NULL; // 初始化为NULL，在getDnsquestion中动态添加
    pmsg->answer = NULL;   // 初始化为NULL，在getDnsanswer中动态添加

    if (debug_mode == 1)
        printf("收到的报文如下：\n");

    // 获取报文头
    buffer = getDnsheader(pmsg, buffer);  // buffer指向读取完报头后的地址
    if(debug_mode==1)
        printHeader(pmsg);

    // 获取询问内容
    buffer = getDnsquestion(pmsg, buffer, start);  // buffer指向读取完询问内容后的地址
    if (debug_mode==1)
        printQuestion(pmsg);

    // 获取应答内容
    buffer = getDnsanswer(pmsg, buffer, start);  // buffer指向读取完应答内容后的地址
    if (debug_mode==1)
        printAnswer(pmsg);
}

// 从缓冲区中读取DNS报文头信息并存储到msg结构体中
uint8_t* getDnsheader(dns_Message* msg, uint8_t* buffer) {
    if (!msg || !msg->header || !buffer) {
        return buffer;
    }

    msg->header->ID = readBits(&buffer, 16); // 获取ID

    uint16_t val = readBits(&buffer, 16); // 获取标志字段
/*
            MSB                                           LSB
            0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
            +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
            |QR|  Opcode   |AA|TC|RD|RA|   Z    |   RCODE   |
            +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
*/

    // 解析标志字段的各个部分
    msg->header->QR = (val & QR_MASK) >> 15;
    msg->header->Opcode = (val & OPCODE_MASK) >> 11;
    msg->header->AA = (val & AA_MASK) >> 10;
    msg->header->TC = (val & TC_MASK) >> 9;
    msg->header->RD = (val & RD_MASK) >> 8;
    msg->header->RA = (val & RA_MASK) >> 7;
    msg->header->RCODE = (val & RCODE_MASK) >> 0;

    // 获取问题数、回答数、权威记录数、附加记录数
    msg->header->QDCOUNT = readBits(&buffer, 16);
    msg->header->ANCOUNT = readBits(&buffer, 16);
    msg->header->NSCOUNT = readBits(&buffer, 16);
    msg->header->ARCOUNT = readBits(&buffer, 16);

    return buffer;
}

// 从缓冲区中读取DNS查询问题并存储到msg结构体中
uint8_t* getDnsquestion(dns_Message* msg, uint8_t* buffer, uint8_t* start) {
    if (!msg || !buffer || !start) {
        return buffer;
    }

    for (int i = 0; i < msg->header->QDCOUNT; ++i) {
        char name[DNS_RR_NAME_MAX_SIZE] = { 0 };
        dns_question* p = malloc(sizeof(dns_question));
        if (!p) continue; // 内存分配失败，尝试下一个

        // 从缓冲区中获取查询域名
        buffer = getDomain(buffer, name, start);

        // 分配内存并复制域名
        p->QNAME = malloc(strlen(name) + 1);
        if (!p->QNAME) {
            free(p);
            continue;
        }
        strcpy(p->QNAME, name);

        p->QTYPE = readBits(&buffer, 16); // 获取查询类型
        p->QCLASS = readBits(&buffer, 16); // 获取查询类
        p->next = NULL;

        // 将新问题插入到问题链表的头部
        if (msg->question) {
            p->next = msg->question;
        }
        msg->question = p;
    }

    return buffer;
}

// 从缓冲区中读取DNS回答信息并存储到msg结构体中
uint8_t* getDnsanswer(dns_Message* msg, uint8_t* buffer, uint8_t* start) {
    if (!msg || !buffer || !start) {
        return buffer;
    }

    for (int i = 0; i < msg->header->ANCOUNT; ++i) {
        char name[DNS_RR_NAME_MAX_SIZE] = { 0 };
        dns_rr* p = malloc(sizeof(dns_rr));
        if (!p) continue; // 内存分配失败，尝试下一个

        // 初始化数据成员
        p->name = NULL;
        p->next = NULL;

        // 从缓冲区中获取回答域名
        buffer = getDomain(buffer, name, start);

        // 分配内存并复制域名
        p->name = malloc(strlen(name) + 1);
        if (!p->name) {
            free(p);
            continue;
        }
        strcpy(p->name, name);

        p->type = readBits(&buffer, 16); // 获取资源记录类型
        p->rrClass = readBits(&buffer, 16); // 获取资源记录类
        p->ttl = readBits(&buffer, 32); // 获取TTL值
        p->rdLength = readBits(&buffer, 16); // 获取资源数据长度

        bool allocation_failed = false;

        // 根据不同的类型进行处理资源数据
        switch (p->type) {
            case DNS_TYPE_A:    // IPv4地址
                for (int j = 0; j < 4; j++) {
                    p->rdata.A_record.address[j] = readBits(&buffer, 8);
                }
                break;
            case DNS_TYPE_AAAA: // IPv6地址
                for (int j = 0; j < 16; j++) {
                    p->rdata.AAAA_record.address[j] = readBits(&buffer, 8);
                }
                break;
            case DNS_TYPE_CNAME: // CNAME记录
                p->rdata.CNAME_record.cname = malloc(p->rdLength + 1);
                if (!p->rdata.CNAME_record.cname) {
                    allocation_failed = true;
                    break;
                }
                memcpy(p->rdata.CNAME_record.cname, buffer, p->rdLength);
                p->rdata.CNAME_record.cname[p->rdLength] = '\0';
                buffer += p->rdLength;
                break;
            case DNS_TYPE_MX: // MX记录
                p->rdata.MX_record.preference = readBits(&buffer, 16);
                p->rdata.MX_record.exchange = malloc(p->rdLength - 2 + 1);
                if (!p->rdata.MX_record.exchange) {
                    allocation_failed = true;
                    break;
                }
                buffer = getDomain(buffer, p->rdata.MX_record.exchange, start);
                break;
            case DNS_TYPE_TXT: // TXT记录
                p->rdata.TXT_record.txt_data = malloc(p->rdLength + 1);
                if (!p->rdata.TXT_record.txt_data) {
                    allocation_failed = true;
                    break;
                }
                memcpy(p->rdata.TXT_record.txt_data, buffer, p->rdLength);
                p->rdata.TXT_record.txt_data[p->rdLength] = '\0';
                buffer += p->rdLength;
                break;
            case DNS_TYPE_SOA: // SOA记录
                p->rdata.SOA_record.mname = malloc(DNS_RR_NAME_MAX_SIZE);
                if (!p->rdata.SOA_record.mname) {
                    allocation_failed = true;
                    break;
                }
                buffer = getDomain(buffer, p->rdata.SOA_record.mname, start);
                
                p->rdata.SOA_record.rname = malloc(DNS_RR_NAME_MAX_SIZE);
                if (!p->rdata.SOA_record.rname) {
                    free(p->rdata.SOA_record.mname);
                    allocation_failed = true;
                    break;
                }
                buffer = getDomain(buffer, p->rdata.SOA_record.rname, start);
                
                p->rdata.SOA_record.serial = readBits(&buffer, 32);
                p->rdata.SOA_record.refresh = readBits(&buffer, 32);
                p->rdata.SOA_record.retry = readBits(&buffer, 32);
                p->rdata.SOA_record.expire = readBits(&buffer, 32);
                p->rdata.SOA_record.minimum = readBits(&buffer, 32);
                break;
            default:    // 其他类型的记录
                buffer += p->rdLength; // 跳过不支持的资源数据
                break;
        }

        if (allocation_failed) {
            free(p->name);
            free(p);
            continue;
        }

        // 将新回答插入到回答链表的头部
        p->next = msg->answer;
        msg->answer = p;
    }
    return buffer;
}

// 从缓冲区中获取域名
uint8_t* getDomain(uint8_t* buffer, char* name, uint8_t* start) {
    if (!buffer || !name || !start) {
        return buffer;
    }

    uint8_t* ptr = buffer;
    int i = 0;
    int len = 0;

    // 若第一个字节的前2位为11，则表示指针，后14位为偏移量，跳转到DNS报文段起始地址 + 偏移量处
    if (*ptr >= 0xc0) {
        uint16_t offset = (*ptr & 0x3f) << 8;
        offset |= *(ptr + 1);  // 获取后14位偏移量
        getDomain(start + offset, name, start);
        return buffer + 2;
    }

    while (*ptr != 0) {
        uint8_t val = *ptr++;

        // 检测压缩指针
        if (val >= 0xc0) {
            uint16_t offset = (val & 0x3f) << 8 | *ptr++;
            if (i > 0) name[i++] = '.';
            uint8_t* savePtr = buffer;
            getDomain(start + offset, name + i, start);
            return ptr; // 返回压缩指针之后的位置
        }

        // 处理标签长度和标签内容
        if (len == 0) {
            len = val;
            if (i > 0) name[i++] = '.';
        } else {
            name[i++] = val;
            len--;
        }
    }

    name[i] = '\0'; // 确保字符串以null结尾
    return ptr + 1; // 跳过结束的0字节
}

// 将指定数量的位（8, 16, 32）设置到缓冲区中
void writeBits(uint8_t** buffer, int bits, int value) {
    switch (bits) {
        case 8:
            **buffer = (uint8_t)value;
            (*buffer)++;
            break;
        case 16: {
            uint16_t val = htons((uint16_t)value);
            memcpy(*buffer, &val, 2);
            *buffer += 2;
            break;
        }
        case 32: {
            uint32_t val = htonl(value);
            memcpy(*buffer, &val, 4);
            *buffer += 4;
            break;
        }
    }
}

// 组装将要发出的DNS报文
uint8_t* dnsstruct_to_str(dns_Message* pmsg, uint8_t* buffer, uint8_t* ip_addr) {
    if (!pmsg || !buffer || !ip_addr) {
        return buffer;
    }

    uint8_t* start = buffer;

    // 组装报头
    buffer = setDnsheader(pmsg, buffer, ip_addr);
    // 组装询问
    buffer = setDnsquestion(pmsg, buffer);
    // 组装回答
    buffer = setDnsanswer(pmsg, buffer, ip_addr);

    return buffer;
}

// 将DNS报文头信息转换为网络字节序并存储到缓冲区中
uint8_t* setDnsheader(dns_Message* msg, uint8_t* buffer, uint8_t* ip_addr) {
    if (!msg || !msg->header || !buffer || !ip_addr) {
        return buffer;
    }

    dns_header* header = msg->header;
    header->QR = 1;       // 设置为回答报文
    header->AA = 1;       // 设置为权威域名服务器
    header->RA = 1;       // 设置为可用递归
    header->QDCOUNT = 1;  // 设置回答数为1

    if (ip_addr[0] == 0 && ip_addr[1] == 0 && ip_addr[2] == 0 && ip_addr[3] == 0) {
        // 若IP地址为0.0.0.0，表示该域名被屏蔽
        header->RCODE = 3;  // 名字错误
    } else {
        header->RCODE = 0;  // 无差错
    }

    writeBits(&buffer, 16, header->ID); // 设置ID

    int flags = 0;
    // 设置标志字段的各个部分
    flags |= (header->QR << 15) & QR_MASK;
    flags |= (header->Opcode << 11) & OPCODE_MASK;
    flags |= (header->AA << 10) & AA_MASK;
    flags |= (header->TC << 9) & TC_MASK;
    flags |= (header->RD << 8) & RD_MASK;
    flags |= (header->RA << 7) & RA_MASK;
    flags |= (header->RCODE << 0) & RCODE_MASK;

    writeBits(&buffer, 16, flags); // 设置标志字段
    writeBits(&buffer, 16, header->QDCOUNT); // 设置问题数
    writeBits(&buffer, 16, header->ANCOUNT); // 设置回答数
    writeBits(&buffer, 16, header->NSCOUNT); // 设置权威记录数
    writeBits(&buffer, 16, header->ARCOUNT); // 设置附加记录数

    return buffer;
}

// 将DNS查询问题转换为网络字节序并存储到缓冲区中
uint8_t* setDnsquestion(dns_Message* msg, uint8_t* buffer) {
    if (!msg || !buffer || !msg->question) {
        return buffer;
    }

    dns_question* p = msg->question;
    
    for (int i = 0; i < msg->header->QDCOUNT && p != NULL; i++) {
        if (p->QNAME) {
            buffer = setDomain(buffer, p->QNAME); // 设置域名
        }

        writeBits(&buffer, 16, p->QTYPE); // 设置查询类型
        writeBits(&buffer, 16, p->QCLASS); // 设置查询类
        
        p = p->next;
    }
    
    return buffer;
}

// 将DNS回答信息转换为网络字节序并存储到缓冲区中
uint8_t* setDnsanswer(dns_Message* msg, uint8_t* buffer, uint8_t* ip_addr) {
    if (!msg || !buffer || !ip_addr || !msg->question || !msg->question->QNAME) {
        return buffer;
    }

    buffer = setDomain(buffer, msg->question->QNAME);

    writeBits(&buffer, 16, 1);  // type (A记录)
    writeBits(&buffer, 16, 1);  // rrClass (IN类)
    writeBits(&buffer, 32, 4);  // ttl (4秒)
    writeBits(&buffer, 16, 4);  // rd_length (IPv4地址长度)

    // 写入IPv4地址
    for (int i = 0; i < 4; i++) {
        *buffer++ = ip_addr[i];
    }

    return buffer;
}

// 将域名转换为网络字节序并存储到缓冲区中
uint8_t* setDomain(uint8_t* buffer, char* name) {
    if (!buffer || !name) {
        return buffer;
    }

    char* ptr = name;
    char label[DNS_RR_NAME_MAX_SIZE] = { 0 };
    int labelLen = 0;

    while (*ptr) {
        if (*ptr == '.') {
            // 写入标签长度和内容
            *buffer++ = labelLen;
            if (labelLen > 0) {
                memcpy(buffer, label, labelLen);
                buffer += labelLen;
            }
            // 重置标签缓冲区
            memset(label, 0, sizeof(label));
            labelLen = 0;
        } else {
            // 将字符添加到当前标签
            if (labelLen < DNS_RR_NAME_MAX_SIZE - 1) {
                label[labelLen++] = *ptr;
            }
        }
        ptr++;
    }

    // 处理最后一个标签
    *buffer++ = labelLen;
    if (labelLen > 0) {
        memcpy(buffer, label, labelLen);
        buffer += labelLen;
    }

    // 域名结束符
    *buffer++ = 0;

    return buffer;
}

// 释放DNS报文结构体所占的内存
void freeMessage(dns_Message* msg) {
    if (!msg) return;

    // 释放头部
    if (msg->header) {
        free(msg->header);
    }

    // 释放问题链表
    dns_question* p = msg->question;
    while (p) {
        dns_question* tmp = p;
        p = p->next;
        
        if (tmp->QNAME) {
            free(tmp->QNAME);
        }
        free(tmp);
    }

    // 释放回答链表
    dns_rr* q = msg->answer;
    while (q) {
        dns_rr* tmp = q;
        q = q->next;
        
        // 根据记录类型释放相应的资源
        switch (tmp->type) {
            case DNS_TYPE_CNAME:
                if (tmp->rdata.CNAME_record.cname)
                    free(tmp->rdata.CNAME_record.cname);
                break;
            case DNS_TYPE_MX:
                if (tmp->rdata.MX_record.exchange)
                    free(tmp->rdata.MX_record.exchange);
                break;
            case DNS_TYPE_TXT:
                if (tmp->rdata.TXT_record.txt_data)
                    free(tmp->rdata.TXT_record.txt_data);
                break;
            case DNS_TYPE_SOA:
                if (tmp->rdata.SOA_record.mname)
                    free(tmp->rdata.SOA_record.mname);
                if (tmp->rdata.SOA_record.rname)
                    free(tmp->rdata.SOA_record.rname);
                break;
        }
        
        if (tmp->name) {
            free(tmp->name);
        }
        free(tmp);
    }

    free(msg);
}






