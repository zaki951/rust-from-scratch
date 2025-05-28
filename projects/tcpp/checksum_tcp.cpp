
#include <iostream>
#include <cstdio>
#include <cassert>

#include "checksum.hpp"

using namespace cks::tcp;

Checksum::Checksum(pseudo_header &psh, const tcphdr &tcph, const std::vector<uint8_t> &payload) : _psh{psh},
                                                                                                  _tcph{tcph},
                                                                                                  _payload{payload}
{
}

std::vector<uint8_t> Checksum::_make_data()
{
    _psh.tcp_length = htons(header_size(_tcph) + _payload.size());

    // Concaténer pseudo-en-tête, en-tête TCP et données
    int total_length = sizeof(_psh) + sizeof(_tcph) + _payload.size();

    std::vector<uint8_t> buffer;
    buffer.resize(total_length);

    memcpy(buffer.data(), &_psh, sizeof(_psh));
    memcpy(buffer.data() + sizeof(_psh), &_tcph, sizeof(_tcph));
    memcpy(buffer.data() + sizeof(_psh) + sizeof(_tcph), _payload.data(), _payload.size());

    return buffer;
}

uint16_t Checksum::_calc(uint16_t *data, uint16_t tlen)
{
    auto shdr = data;
    auto pkt = data + 6;
    uint16_t pad = 0;

    uint32_t csum = shdr[0];

    csum += shdr[1] + shdr[2] + shdr[3] + htons(6) + htons(tlen);

    csum += pkt[0] + pkt[1] + pkt[2] + pkt[3] + pkt[4] + pkt[5] + pkt[6] +
            pkt[7] + pkt[9];

    tlen -= 20;
    pkt += 10;

    while (tlen >= 32)
    {
        csum += pkt[0] + pkt[1] + pkt[2] + pkt[3] + pkt[4] + pkt[5] + pkt[6] +
                pkt[7] + pkt[8] + pkt[9] + pkt[10] + pkt[11] + pkt[12] + pkt[13] +
                pkt[14] + pkt[15];
        tlen -= 32;
        pkt += 16;
    }

    while (tlen >= 8)
    {
        csum += pkt[0] + pkt[1] + pkt[2] + pkt[3];
        tlen -= 8;
        pkt += 4;
    }

    while (tlen >= 4)
    {
        csum += pkt[0] + pkt[1];
        tlen -= 4;
        pkt += 2;
    }

    while (tlen > 1)
    {
        csum += pkt[0];
        pkt += 1;
        tlen -= 2;
    }

    if (tlen == 1)
    {
        *(uint8_t *)(&pad) = (*(uint8_t *)pkt);
        csum += pad;
    }

    csum = (csum >> 16) + (csum & 0x0000FFFF);
    csum += (csum >> 16);

    return (uint16_t)~csum;
}

uint16_t calc_tcp_checksum(const ip &iph, tcphdr &tcph, const std::vector<uint8_t> &payload)
{
    using namespace cks::tcp;

    if (payload.size())
        std::cout << "calc_tcp_checksum(payload)" << (char *)payload.data() << std::endl;
    pseudo_header psh;
    psh.src_addr = iph.ip_src.s_addr;
    psh.dest_addr = iph.ip_dst.s_addr;

    Checksum ck{psh, tcph, payload};
    return ck.calc();
}

uint16_t Checksum::calc()
{
    auto buff = _make_data();
#ifdef DEBUG_CK
    std::cout << buff.size() << std::endl;
    for (auto c : buff)
    {
        printf("%x ", c);
    }
    std::cout << std::endl;
#endif

    return _calc(reinterpret_cast<uint16_t *>(buff.data()), _tcph.th_off * 4 + _payload.size());
}

using FuncTest = decltype([]() -> auto {});

template <typename Func = FuncTest>
std::pair<std::string, Func> make_test(const std::string &name, Func func)
{
    return {name, func};
}

#define SUCCESS_TEST(name) std::cout << std::format("[TEST: {}] SUCCESS\n", name)

#define FAIL_TEST(name) std::cout << std::format("[TEST: {}] FAIL\n", name)

#define RUN(test)                                         \
    do                                                    \
    {                                                     \
        auto &[name, routine] = test;                     \
        routine() ? SUCCESS_TEST(name) : FAIL_TEST(name); \
    } while (0)

#ifdef TEST_CK

std::vector tests{
    make_test("TEST A", [] -> bool
              {
        pseudo_header psh;
        psh.src_addr = inet_addr("192.168.1.1");   
        psh.dest_addr = inet_addr("192.168.1.2"); 
        psh.zero = 0;
        psh.protocol = IPPROTO_TCP;
        
        tcphdr tcph;
        memset(&tcph, 0, sizeof(tcph));
        tcph.source = htons(12345); 
        tcph.dest = htons(80);     
        tcph.seq = htonl(1);       
        tcph.doff = 5;              

        std::vector<uint8_t> payload {'H', 'e', 'l', 'l', 'o', ',', ' ', 'T', 'C', 'P', '!' };

        Checksum ck {psh, tcph, payload};
        std::cout << std::hex << Checksum::to_host(ck.calc()) << std::endl;
        printf("TCP Checksum: 0x%04x\n", ck.calc()) ;
        return ntohs(ck.calc()) == 0x5359; })};

int main()
{
    for (auto test : tests)
    {
        RUN(test);
    }

    return 0;
}
#endif
