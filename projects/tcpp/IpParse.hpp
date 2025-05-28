#pragma once

#include <iostream>
#include <ranges>
#include <format>
#include <vector>
#include <cassert>
#include <cstring>
#include <span>

#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

const ip *parse_ipv4_packet(const std::vector<char> &buffer);

// struct ip {
// #if BYTE_ORDER == LITTLE_ENDIAN
// 	u_char	ip_hl:4,		/* header length */
// 		ip_v:4;			/* version */
// #endif
// #if BYTE_ORDER == BIG_ENDIAN
// 	u_char	ip_v:4,			/* version */
// 		ip_hl:4;		/* header length */
// #endif
// 	u_char	ip_tos;			/* type of service */
// 	u_short	ip_len;			/* total length */
// 	u_short	ip_id;			/* identification */
// 	u_short	ip_off;			/* fragment offset field */
// #define	IP_RF 0x8000			/* reserved fragment flag */
// #define	IP_DF 0x4000			/* dont fragment flag */
// #define	IP_MF 0x2000			/* more fragments flag */
// #define	IP_OFFMASK 0x1fff		/* mask for fragmenting bits */
// 	u_char	ip_ttl;			/* time to live */
// 	u_char	ip_p;			/* protocol */
// 	u_short	ip_sum;			/* checksum */
// 	struct	in_addr ip_src,ip_dst;	/* source and dest address */
// } __packed __aligned(4);

// struct tcphdr {
// 	__be16	source;
// 	__be16	dest;
// 	__be32	seq;
// 	__be32	ack_seq;
// #if defined(__LITTLE_ENDIAN_BITFIELD)
// 	__u16	res1:4,
// 		doff:4,
// 		fin:1,
// 		syn:1,
// 		rst:1,
// 		psh:1,
// 		ack:1,
// 		urg:1,
// 		ece:1,
// 		cwr:1;
// #elif defined(__BIG_ENDIAN_BITFIELD)
// 	__u16	doff:4,
// 		res1:4,
// 		cwr:1,
// 		ece:1,
// 		urg:1,
// 		ack:1,
// 		psh:1,
// 		rst:1,
// 		syn:1,
// 		fin:1;
// #else
// #error	"Adjust your <asm/byteorder.h> defines"
// #endif
// 	__be16	window;
// 	__sum16	check;
// 	__be16	urg_ptr;
// };

const tcphdr *parse_tcp_packet(const std::vector<char> &buffer);

size_t header_size(const ip &i);

size_t header_size(const tcphdr &t);

size_t write_into_buffer(const tcphdr &header, std::span<uint8_t> &buff);

size_t write_into_buffer(const ip &header, std::span<uint8_t> &buff);

template <typename T>
size_t write_into_buffer(const std::vector<T> &src, std::span<uint8_t> &dst)
{
    const size_t written{sizeof(T) * src.size()};
    assert(written < dst.size());
    std::memcpy(dst.data(), src.data(), written);
    return written;
}

uint16_t checksum(uint16_t *pkt, uint32_t hlen);

ip make_iph(uint16_t payload_len,
            uint8_t time_to_live,
            uint8_t proto,
            in_addr src,
            in_addr dst);
