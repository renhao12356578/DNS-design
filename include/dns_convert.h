#pragma once
#include "dns_struct.h"

// 用于获取DNS报文头各值的掩码
static const uint32_t QR_MASK = 0x8000;
static const uint32_t OPCODE_MASK = 0x7800;
static const uint32_t AA_MASK = 0x0400;
static const uint32_t TC_MASK = 0x0200;
static const uint32_t RD_MASK = 0x0100;
static const uint32_t RA_MASK = 0x0080;
static const uint32_t RCODE_MASK = 0x000F;

// DNS 报文字串转换为 DNS 结构体
void str_to_dnsstruct(dns_Message* pmsg, uint8_t* buffer, uint8_t* start);

// DNS 结构体转换为 DNS 报文字串
uint8_t* dnsstruct_to_str(dns_Message* pmsg, uint8_t* buffer, uint8_t* ip_addr);

// 从字串获取header
uint8_t* getDnsheader(dns_Message* msg, uint8_t* buffer);

// 从字串获取question
uint8_t* getDnsquestion(dns_Message* msg, uint8_t* buffer, uint8_t* start);

// 从字串获取 answer
uint8_t* getDnsanswer(dns_Message* msg, uint8_t* buffer, uint8_t* start);

// 从字串获取域名Name
uint8_t* getDomain(uint8_t* buffer, char* name, uint8_t* start);

// 写入指定位数
void writeBits(uint8_t** buffer, int bits, int value);

// 将header写入字串
uint8_t* setDnsheader(dns_Message* msg, uint8_t* buffer, uint8_t* ip_addr);

// 将question写入字串
uint8_t* setDnsquestion(dns_Message* msg, uint8_t* buffer);

// 将answer写入字串
uint8_t* setDnsanswer(dns_Message* msg, uint8_t* buffer, uint8_t* ip_addr);

// 设置域名
uint8_t* setDomain(uint8_t* buffer, char* name);

// 释放空间
void freeMessage(dns_Message* msg);
