#include "IpParse.hpp"

const ip *parse_ipv4_packet(const std::vector<char> &buffer)
{

    if (buffer.size() < sizeof(ip))
    {
        return nullptr;
    }

    const ip *ip_header = reinterpret_cast<const ip *>(buffer.data());

    if (ip_header->ip_v != 4)
    {
        return nullptr;
    }

    // std::cout << "Version: " << (int)ip_header->ip_v << std::endl;
    // std::cout << "Longueur de l'en-tête (en 4-octets): " << (int)ip_header->ip_hl << std::endl;
    // std::cout << "Adresse IP source: " << inet_ntoa(ip_header->ip_src) << std::endl;
    // std::cout << "Adresse IP destination: " << inet_ntoa(ip_header->ip_dst) << std::endl;
    // std::cout << "Protocole: " << (int)ip_header->ip_p << std::endl;

    // std::cout << "Type de service: " << (int)ip_header->ip_tos << std::endl;
    // std::cout << "Longueur totale du paquet: " << ntohs(ip_header->ip_len) << " octets" << std::endl;
    // std::cout << "TTL: " << (int)ip_header->ip_ttl << std::endl;
    // std::cout << "Identifiant: " << ntohs(ip_header->ip_id) << std::endl;
    // std::cout << "Fragmentation offset: " << ntohs(ip_header->ip_off) << std::endl;
    return ip_header;
}

const tcphdr *parse_tcp_packet(const std::vector<char> &buffer)
{
    if (buffer.size() < sizeof(struct ip))
    {
        return nullptr;
    }

    ip *ip_header = reinterpret_cast<ip *>(const_cast<char *>(buffer.data()));

    const tcphdr *tcp_header = reinterpret_cast<const tcphdr *>(buffer.data() + ip_header->ip_hl * 4);
    if (!tcp_header)
        return nullptr;

    // // Affichage des informations TCP
    // std::cout << "Source Port: " << ntohs(tcp_header->th_sport) << std::endl;
    // std::cout << "Destination Port: " << ntohs(tcp_header->th_dport) << std::endl;
    // std::cout << "Sequence Number: " << ntohl(tcp_header->th_seq) << std::endl;
    // std::cout << "Acknowledgment Number: " << ntohl(tcp_header->th_ack) << std::endl;
    // std::cout << std::format("flags psh({}), syn({}, ack({}))",
    // tcp_header->psh,
    // tcp_header->syn,
    // tcp_header->ack) << std::endl;

    return tcp_header;
}

size_t header_size(const ip &i)
{
    return i.ip_hl * 4;
}

size_t header_size(const tcphdr &t)
{
    return t.th_off * 4;
}

size_t write_into_buffer(const auto &header, std::span<uint8_t> &buff)
{
    if (buff.size() < sizeof(header))
    {
        throw std::runtime_error("The buffer is too small");
    }
    std::memcpy(buff.data(), &header, sizeof(header));
    return sizeof(header);
}

size_t write_into_buffer(const ip &header, std::span<uint8_t> &buff)
{
    if (buff.size() < sizeof(header))
    {
        throw std::runtime_error("The buffer is too small");
    }
    std::memcpy(buff.data(), &header, sizeof(header));
    return sizeof(header);
}

uint16_t checksum(uint16_t *pkt, uint32_t hlen)
{
    uint32_t csum = pkt[0];

    csum += pkt[1] + pkt[2] + pkt[3] + pkt[4] + pkt[6] + pkt[7] + pkt[8] +
            pkt[9];

    hlen -= 20;
    pkt += 10;

    if (hlen == 0)
    {
        ;
    }
    else if (hlen == 4)
    {
        csum += pkt[0] + pkt[1];
    }
    else if (hlen == 8)
    {
        csum += pkt[0] + pkt[1] + pkt[2] + pkt[3];
    }
    else if (hlen == 12)
    {
        csum += pkt[0] + pkt[1] + pkt[2] + pkt[3] + pkt[4] + pkt[5];
    }
    else if (hlen == 16)
    {
        csum += pkt[0] + pkt[1] + pkt[2] + pkt[3] + pkt[4] + pkt[5] + pkt[6] +
                pkt[7];
    }
    else if (hlen == 20)
    {
        csum += pkt[0] + pkt[1] + pkt[2] + pkt[3] + pkt[4] + pkt[5] + pkt[6] +
                pkt[7] + pkt[8] + pkt[9];
    }
    else if (hlen == 24)
    {
        csum += pkt[0] + pkt[1] + pkt[2] + pkt[3] + pkt[4] + pkt[5] + pkt[6] +
                pkt[7] + pkt[8] + pkt[9] + pkt[10] + pkt[11];
    }
    else if (hlen == 28)
    {
        csum += pkt[0] + pkt[1] + pkt[2] + pkt[3] + pkt[4] + pkt[5] + pkt[6] +
                pkt[7] + pkt[8] + pkt[9] + pkt[10] + pkt[11] + pkt[12] + pkt[13];
    }
    else if (hlen == 32)
    {
        csum += pkt[0] + pkt[1] + pkt[2] + pkt[3] + pkt[4] + pkt[5] + pkt[6] +
                pkt[7] + pkt[8] + pkt[9] + pkt[10] + pkt[11] + pkt[12] + pkt[13] +
                pkt[14] + pkt[15];
    }
    else if (hlen == 36)
    {
        csum += pkt[0] + pkt[1] + pkt[2] + pkt[3] + pkt[4] + pkt[5] + pkt[6] +
                pkt[7] + pkt[8] + pkt[9] + pkt[10] + pkt[11] + pkt[12] + pkt[13] +
                pkt[14] + pkt[15] + pkt[16] + pkt[17];
    }
    else if (hlen == 40)
    {
        csum += pkt[0] + pkt[1] + pkt[2] + pkt[3] + pkt[4] + pkt[5] + pkt[6] +
                pkt[7] + pkt[8] + pkt[9] + pkt[10] + pkt[11] + pkt[12] + pkt[13] +
                pkt[14] + pkt[15] + pkt[16] + pkt[17] + pkt[18] + pkt[19];
    }

    csum = (csum >> 16) + (csum & 0x0000FFFF);
    csum += (csum >> 16);

    return (uint16_t)~csum;
}

ip make_iph(uint16_t payload_len,
            uint8_t time_to_live,
            uint8_t proto,
            in_addr src,
            in_addr dst)
{
    ip iph;
    std::memset(&iph, 0, sizeof(ip));
    iph.ip_ttl = time_to_live;

    iph.ip_dst.s_addr = dst.s_addr;
    iph.ip_src.s_addr = src.s_addr;
    iph.ip_off = htons(IP_DF);
    iph.ip_p = proto;
    iph.ip_len = htons(sizeof(ip));
    iph.ip_id = htons(12345);
    iph.ip_v = 4;
    iph.ip_hl = 5;
    iph.ip_sum = 0; // checksum(reinterpret_cast<uint16_t*>(&iph), sizeof(ip));
    return iph;
}
