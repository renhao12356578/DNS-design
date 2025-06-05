//打印DNS报文
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
void printRR(const dns_rr* rr) {
    if (!rr) return;

    // 打印通用的RR头部信息
    log_message(LOG_DEBUG, "Name (domin): %s, TTL: %u, Class: %s, Type: %s, Data Length: %u",
        rr->name,
        rr->ttl,
        (rr->rrClass == DNS_CLASS_IN) ? "IN" : "Unknown",
        type_to_string(rr->type),
        rr->rdLength);

    // 打印特定类型的RDATA
    switch (rr->type) {
        case DNS_TYPE_A: {   //ipv4
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, rr->rdata.A_record.address, ip_str, INET_ADDRSTRLEN);
            log_message(LOG_DEBUG, "RDATA: Address: %s", ip_str);
            break;
        }
        case DNS_TYPE_AAAA: {
            char ip_str[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, rr->rdata.AAAA_record.address, ip_str, INET6_ADDRSTRLEN);
            log_message(LOG_DEBUG, "RDATA: Address: %s", ip_str);
            break;
        }
        case DNS_TYPE_CNAME:
            log_message(LOG_DEBUG, "RDATA: CNAME: %s", rr->rdata.CNAME_record.cname);
            break;
        case DNS_TYPE_NS:
            log_message(LOG_DEBUG, "RDATA: Name Server: %s", rr->rdata.NS_record.nsdname);
            break;
        case DNS_TYPE_PTR:
            log_message(LOG_DEBUG, "RDATA: Pointer: %s", rr->rdata.PTR_record.ptrdname);
            break;
        case DNS_TYPE_MX:
            log_message(LOG_DEBUG, "RDATA: Preference: %u, Mail Exchange: %s",
                rr->rdata.MX_record.preference, rr->rdata.MX_record.exchange);
            break;
        case DNS_TYPE_TXT:
            log_message(LOG_DEBUG, "RDATA: Text: \"%s\"", rr->rdata.TXT_record.txt_data);
            break;
        case DNS_TYPE_SOA:
            log_message(LOG_DEBUG, "RDATA: Primary Name Server: %s", rr->rdata.SOA_record.mname);
            log_message(LOG_DEBUG, "RDATA: Responsible Mailbox: %s", rr->rdata.SOA_record.rname);
            log_message(LOG_DEBUG, "RDATA: Serial: %u, Refresh: %u, Retry: %u, Expire: %u, Minimum TTL: %u",
                rr->rdata.SOA_record.serial, rr->rdata.SOA_record.refresh,
                rr->rdata.SOA_record.retry, rr->rdata.SOA_record.expire,
                rr->rdata.SOA_record.minimum);
            break;
        default:
            log_message(LOG_DEBUG, "RDATA: Data format not implemented for printing.");
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
    log_message(LOG_DEBUG, "--- DNS Message Raw Data (Length: %u bytes) ---", length);
    char line[128] = {0};
    char ascii[17] = {0};
    int pos = 0;
    
    for (unsigned int i = 0; i < length; ++i) {
        // 添加十六进制值
        pos += snprintf(line + pos, sizeof(line) - pos, "%02X ", (unsigned char)pstring[i]);
        ascii[i % 16] = isprint((unsigned char)pstring[i]) ? (unsigned char)pstring[i] : '.';
        
        // 每16个字节输出一行
        if ((i + 1) % 16 == 0 || (i + 1) == length) {
            // 补齐空格
            while (pos < 48) {
                pos += snprintf(line + pos, sizeof(line) - pos, "   ");
            }
            // 添加ASCII部分
            ascii[i % 16 + 1] = '\0';
            snprintf(line + pos, sizeof(line) - pos, "| %s", ascii);
            log_message(LOG_DEBUG, "%s", line);
            pos = 0;
            memset(line, 0, sizeof(line));
            memset(ascii, 0, sizeof(ascii));
        }
    }
    log_message(LOG_DEBUG, "------------------------------------------------");
}

/**
 * @brief 打印DNS报文的头部信息。
 * @param msg 指向包含已解析报文的dns_Message结构体。
 */
void printHeader(dns_Message* msg) {
    if (!msg || !msg->header) {
        log_message(LOG_DEBUG, "Header is NULL.");
        return;
    }
    dns_header* h = msg->header;
    log_message(LOG_DEBUG, "----------------------------header----------------------------");
    log_message(LOG_DEBUG, "Transaction ID: 0x%04X (%u)", h->ID, h->ID);

    // Flags
    log_message(LOG_DEBUG, "Flags:");
    log_message(LOG_DEBUG, "QR=%d (Response)", h->QR);
    log_message(LOG_DEBUG, "Opcode=%d", h->Opcode);
    log_message(LOG_DEBUG, "AA = %d", h->AA);
    log_message(LOG_DEBUG, "TC = %d", h->TC);
    log_message(LOG_DEBUG, "RD = %d", h->RD);
    log_message(LOG_DEBUG, "RA = %d", h->RA);
    log_message(LOG_DEBUG, "rcode = %d (%s)", h->RCODE, 
               (h->RCODE == DNS_RCODE_OK) ? "No error" : "Error");

    // Counts
    log_message(LOG_DEBUG, "Questions: %u", msg->header->QDCOUNT);
    log_message(LOG_DEBUG, "Answer RRs: %u", msg->header->ANCOUNT);
    log_message(LOG_DEBUG, "Authority RRs: %u", msg->header->NSCOUNT);
    log_message(LOG_DEBUG, "Additional RRs: %u", msg->header->ARCOUNT);
    log_message(LOG_DEBUG, "----------------------------------------------------------------");
}

/**
 * @brief 打印DNS报文的问题部分。
 * @param msg 指向包含已解析报文的dns_Message结构体。
 */
void printQuestion(dns_Message* msg) {
    if (!msg || !msg->question) {
        log_message(LOG_DEBUG, "Question section is NULL.");
        return;
    }
    dns_question* q = msg->question;
    log_message(LOG_DEBUG, "---------------------------- DNS Question----------------------------");
    log_message(LOG_DEBUG, "  Name: %s", q->QNAME);
    log_message(LOG_DEBUG, "  Type: %s (%u)", type_to_string(q->QTYPE), q->QTYPE);
    log_message(LOG_DEBUG, "  Class: %s (%u)", 
               (q->QCLASS == DNS_CLASS_IN) ? "IN" : "Unknown", q->QCLASS);
    log_message(LOG_DEBUG, "--------------------------------------------------------------------");
}

/**
 * @brief 打印DNS报文的回答部分，会遍历整个资源记录链表。
 * @param msg 指向包含已解析报文的dns_Message结构体。
 */
void printAnswer(dns_Message* msg) {
    if (!msg || !msg->answer || msg->header->ANCOUNT == 0) {
        log_message(LOG_DEBUG, "Answer section is empty or NULL.");
        return;
    }
    dns_rr* rr = msg->answer;
    log_message(LOG_DEBUG, "-------------------------DNS Answer Section-------------------------");
    int count = 1;
    while (rr) {
        log_message(LOG_DEBUG, "[%d]", count++);
        printRR(rr);
        rr = rr->next;
    }
    log_message(LOG_DEBUG, "--------------------------------------------------------------------");
}

/**
 * @brief 答应问题和回答数以及问题和回答种类
 */
void printQuestionAndAnswer(dns_Message msg){
    log_message(LOG_DEBUG, "[QDCOUNT: %d], [ANCOUNT: %d]", msg.header->QDCOUNT,msg.header->ANCOUNT);
    log_message(LOG_DEBUG, "Question type [%s]", type_to_string(msg.question->QTYPE));

    int count = 1;
    dns_rr* answer = msg.answer;
    while(count <= msg.header->ANCOUNT&&answer != NULL){
        log_message(LOG_DEBUG, "Answer %d type [%s]", count,type_to_string(answer->type));
        answer = answer -> next;
        count++;
    }
}

