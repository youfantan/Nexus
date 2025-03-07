#pragma once

#include <iostream>
#include "./win32_io.h"
#include "./include/utils/unexpected.h"

namespace Nexus::Utils {
    static MayFail<in_addr> DNSLookUpV4(const std::string& str) {
        PDNS_RECORD dnsrec {};
        DNS_STATUS status = DnsQuery(str.c_str(), DNS_TYPE_A, DNS_QUERY_STANDARD, nullptr, &dnsrec, nullptr);
        if (status == DNS_ERROR_RCODE_NO_ERROR) {
            for (PDNS_RECORD r = dnsrec; r != nullptr; r = r->pNext) {
                if (r->wType == DNS_TYPE_A) {
                    in_addr adr{};
                    adr.S_un.S_addr = r->Data.A.IpAddress;
                    char straddr[16] = {};
                    inet_ntop(AF_INET, reinterpret_cast<void*>(&adr), straddr, 16);
                    DnsRecordListFree(dnsrec, DnsFreeRecordList);
                    return adr;
                }
            }
        }
        BREAKPOINT;
        return failed;
    }
    static MayFail<in6_addr> DNSLookUpV6(const std::string& str) {
        PDNS_RECORD dnsrec {};
            DNS_STATUS status = DnsQuery(str.c_str(), DNS_TYPE_AAAA, DNS_QUERY_STANDARD, nullptr, &dnsrec, nullptr);
        if (status == DNS_ERROR_RCODE_NO_ERROR) {
            for (PDNS_RECORD r = dnsrec; r != nullptr; r = r->pNext) {
                if (r->wType == DNS_TYPE_AAAA) {
                    in6_addr adr{};
                    memcpy(adr.u.Byte, r->Data.AAAA.Ip6Address.IP6Byte, sizeof(in6_addr));
                    char straddr[46] = {};
                    inet_ntop(AF_INET6, reinterpret_cast<void*>(&adr), straddr, 46);
                    DnsRecordListFree(dnsrec, DnsFreeRecordList);
                    return adr;
                }
            }
        }
        BREAKPOINT;
        return failed;
    }
}