//打印DNS报文
#include "dns_struct.h"
#include  "dns_mes_print.h"
#include <ctype.h> 
// For isprint()

// --- Helper Functions ---

/**
 * @brief 将DNS记录类型码转换为字符串。
 * @param type 类型码。
 * @return 代表类型的字符串。
 */
const char* type_to_string(uint16_t type) {
    switch (type) {
        case DNS_TYPE_A: return "A";
        case DNS_TYPE_NS: return "NS";
        case DNS_TYPE_CNAME: return "CNAME";
        case DNS_TYPE_SOA: return "SOA";
        case DNS_TYPE_PTR: return "PTR";
        case DNS_TYPE_HINFO: return "HINFO";
        case DNS_TYPE_MINFO: return "MINFO";
        case DNS_TYPE_MX: return "MX";
        case DNS_TYPE_TXT: return "TXT";
        case DNS_TYPE_AAAA: return "AAAA";
        default: return "Unknown";
    }
}

/**
 * @brief 打印单个资源记录 (RR)。
 * @param rr 指向要打印的 dns_RR 结构体的指针。
 */
void printRR(const dns_RR* rr) {
    if (!rr) return;

    // 打印通用的RR头部信息
    printf("  Name: %s, TTL: %u, Class: %s, Type: %s, Data Length: %u\n",
        rr->name,
        rr->ttl,
        (rr->rr_class == DNS_CLASS_IN) ? "IN" : "Unknown",
        type_to_string(rr->type),
        rr->rdlength);

    // 打印特定类型的RDATA
    printf("    RDATA: ");
    switch (rr->type) {
        case DNS_TYPE_A: {
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, rr->rdata.A_record.address, ip_str, INET_ADDRSTRLEN);
            printf("Address: %s\n", ip_str);
            break;
        }
        case DNS_TYPE_AAAA: {
            char ip_str[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, rr->rdata.AAAA_record.address, ip_str, INET6_ADDRSTRLEN);
            printf("Address: %s\n", ip_str);
            break;
        }
        case DNS_TYPE_CNAME:
            printf("CNAME: %s\n", rr->rdata.CNAME_record.cname);
            break;
        case DNS_TYPE_NS:
            printf("Name Server: %s\n", rr->rdata.NS_record.nsdname);
            break;
        case DNS_TYPE_PTR:
            printf("Pointer: %s\n", rr->rdata.PTR_record.ptrdname);
            break;
        case DNS_TYPE_MX:
            printf("Preference: %u, Mail Exchange: %s\n",
                rr->rdata.MX_record.preference, rr->rdata.MX_record.exchange);
            break;
        case DNS_TYPE_TXT:
             // 假设TXT数据是单个C字符串
            printf("Text: \"%s\"\n", rr->rdata.TXT_record.txt_data);
            break;
        case DNS_TYPE_SOA:
            printf("\n");
            printf("      Primary Name Server: %s\n", rr->rdata.SOA_record.mname);
            printf("      Responsible Mailbox: %s\n", rr->rdata.SOA_record.rname);
            printf("      Serial: %u, Refresh: %u, Retry: %u, Expire: %u, Minimum TTL: %u\n",
                rr->rdata.SOA_record.serial, rr->rdata.SOA_record.refresh,
                rr->rdata.SOA_record.retry, rr->rdata.SOA_record.expire,
                rr->rdata.SOA_record.minimum);
            break;
        default:
            printf("Data format not implemented for printing.\n");
            break;
    }
}


// --- Main Implementation Functions ---

/**
 * @brief 以十六进制和ASCII格式打印DNS报文的字节流。
 * @param pstring 指向字节流的指针。
 * @param length 字节流的长度。
 */
void printDnstring(char* pstring, unsigned int length) {
    printf("--- DNS Message Raw Data (Length: %u bytes) ---\n", length);
    for (unsigned int i = 0; i < length; ++i) {
        // 打印十六进制值
        printf("%02X ", (unsigned char)pstring[i]);
        
        // 每16个字节换行并打印ASCII值
        if ((i + 1) % 16 == 0 || (i + 1) == length) {
            // 补齐空格
            if ((i + 1) % 16 != 0) {
                for (unsigned int j = 0; j < 16 - ((i + 1) % 16); ++j) {
                    printf("   ");
                }
            }
            printf(" | ");
            
            // 打印ASCII
            unsigned int start = (i / 16) * 16;
            for (unsigned int j = start; j <= i; ++j) {
                printf("%c", isprint((unsigned char)pstring[j]) ? (unsigned char)pstring[j] : '.');
            }
            printf("\n");
        }
    }
    printf("------------------------------------------------\n\n");
}

/**
 * @brief 打印DNS报文的头部信息。
 * @param msg 指向包含已解析报文的dns_Message结构体。
 */
void printHeader(dns_Message* msg) {
    if (!msg || !msg->header) {
        printf("Header is NULL.\n");
        return;
    }
    dns_header* h=msg->header;
    printf("----------------------------header----------------------------\n");
    printf("Transaction ID: 0x%04X (%u)\n",h->ID, h->ID);

    // Flags
    printf("Flags: \n");
    printf("QR=%d (Response),\n ", h->QR);
    printf("Opcode=%d,\n", h->Opcode);
    printf("AA = %d,\n ", h->AA);
    printf("TC = %d, \n", h->TC);
    printf("RD = %d, \n", h->RD);
    printf("RA = %d\n", h->RA);
    printf("RCODE = %d(%s),\n ", h->Z);
    printf("rcode = %d\n", h->RCODE,(h->RCODE == DNS_RCODE_OK) ? "No error" : "Error");


    // Counts
    printf("Questions: %u\n", msg->header->QDCOUNT);
    printf("Answer RRs: %u\n", msg->header->ANCOUNT);
    printf("Authority RRs: %u\n", msg->header->NSCOUNT);
    printf("Additional RRs: %u\n", msg->header->ARCOUNT);
    printf("----------------------------------------------------------------\n\n");
}


/**
 * @brief 打印DNS报文的问题部分。
 * @param msg 指向包含已解析报文的dns_Message结构体。
 */
void printQuestion(dns_Message* msg) {
    if (!msg || !msg->question) {
        printf("Question section is NULL.\n");
        return;
    }
    dns_question* q = msg->question;
    printf("---------------------------- DNS Question----------------------------\n");
    printf("  Name: %s\n", q->QNAME);
    printf("  Type: %s (%u)\n", type_to_string(q->QTYPE), q->QTYPE);
    printf("  Class: %s (%u)\n", (q->QCLASS == DNS_CLASS_IN) ? "IN" : "Unknown", q->QCLASS);
    printf("--------------------------------------------------------------------\n\n");
}

/**
 * @brief 打印DNS报文的回答部分，会遍历整个资源记录链表。
 * @param msg 指向包含已解析报文的dns_Message结构体。
 */
void printAnswer(dns_Message* msg) {
    if (!msg || !msg->answer) {
        printf("Answer section is empty or NULL.\n\n");
        return;
    }
    dns_RR* rr = msg->answer;
    printf("-------------------------DNS Answer Section-------------------------\n");
    int count = 1;
    while (rr) {
        printf("[%d]\n", count++);
        printRR(rr);
        rr = rr->next;
    }
    printf("--------------------------------------------------------------------\n\n");
}


// --- Demo Main Function ---
// 编译和运行时，此部分会展示上述函数的用法。
int main() {
    printf("--- DNS Message Print Demo ---\n\n");
    
    // 1. 创建并填充模拟的DNS报文结构
    // (在实际应用中，这些结构体是通过解析网络字节流填充的)
    
    // Header
    dns_header h = {0};
    h.ID = 0x1234;
    h.QRL = DNS_QR_ANSWER;
    h.Opcode = DNS_OPCODE_QUERY;
    h.AA = 0;
    h.TC = 0;
    h.RD = 1;
    h.RA = 1;
    h.Z = 0;
    h.RCODE = DNS_RCODE_OK;
    h.QDCOUNT = 1;
    h.ANCOUNT = 1;
    h.NSCOUNT = 0;
    h.ARCOUNT = 0;

    // Question
    dns_question q = {0};
    q.QNAME = "www.google.com";
    q.QTYPE = DNS_TYPE_A;
    q.QCLASS = DNS_CLASS_IN;
    
    // Answer RR (单个A记录)
    dns_RR ans_rr = {0};
    ans_rr.name = "www.google.com";
    ans_rr.type = DNS_TYPE_A;
    ans_rr.rr_class = DNS_CLASS_IN;
    ans_rr.ttl = 180; // 3 minutes
    ans_rr.rdlength = 4;
    // Google's IP 142.250.184.132
    ans_rr.rdata.A_record.address[0] = 142;
    ans_rr.rdata.A_record.address[1] = 250;
    ans_rr.rdata.A_record.address[2] = 184;
    ans_rr.rdata.A_record.address[3] = 132;
    ans_rr.next = NULL; // 只有一个回答记录

    // Top-level message struct
    dns_Message msg = {0};
    msg.header = &h;
    msg.question = &q;
    msg.answer = &ans_rr;
    msg.authority = NULL;
    msg.additional = NULL;
    
    // 2. 调用打印函数
    printHeader(&msg);
    printQuestion(&msg);
    printAnswer(&msg);

    // 3. 演示原始字节流打印
    char example_stream[] = {
        0x12, 0x34, // ID
        0x81, 0x80, // Flags
        0x00, 0x01, // QDCOUNT
        0x00, 0x01, // ANCOUNT
        0x00, 0x00, // NSCOUNT
        0x00, 0x00  // ARCOUNT
    };
    printDnstring(example_stream, sizeof(example_stream));
    
    return 0;
}