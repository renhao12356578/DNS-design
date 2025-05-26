#pragma once
#include "DNS_struct.h"

// 打印 DNS 报文字节流
void printDnstring(char* pstring, unsigned int length);

// 打印Header
void printHeader(dns_Message* msg);

// 打印Question
void printQuestion(dns_Message* msg);

// 打印Answer（RR）
void printAnswer(dns_Message* msg);
