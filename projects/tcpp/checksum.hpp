#pragma once

#include <vector>
#include <stdio.h>
#include <cstdint>
#include <cstring>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "IpParse.hpp"

namespace cks::tcp
{

    struct pseudo_header
    {
        uint32_t src_addr;
        uint32_t dest_addr;
        uint8_t zero{0};
        uint8_t protocol{IPPROTO_TCP};
        uint16_t tcp_length;
    };

    class Checksum
    {
    public:
        Checksum(pseudo_header &psh, const tcphdr &tcp, const std::vector<uint8_t> &payload);
        uint16_t calc();
        constexpr static uint16_t to_host(auto n) { return ntohs(n); }

    private:
        uint16_t _calc(uint16_t *data, uint16_t length);
        std::vector<uint8_t> _make_data();

    private:
        pseudo_header &_psh;
        const tcphdr &_tcph;
        const std::vector<uint8_t> &_payload;
    };

}

uint16_t calc_tcp_checksum(const ip &iph, tcphdr &tcph, const std::vector<uint8_t> &payload);