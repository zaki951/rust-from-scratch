#pragma once

#include <optional>
#include <chrono>
#include <span>
#include <map>
#include <ranges>
#include "Helper.hpp"
#include "Tuntap.hpp"
#include "Interface.hpp"
#include "IpParse.hpp"
#include "checksum.hpp"

namespace tcp
{

    enum class Available
    {
        READ,
        WRITE
    };

    struct TcpStream
    {
    };

    struct RecvSequenceSpace
    {
        /// receive next
        uint32_t nxt{0};
        /// receive window
        uint16_t wnd{0};
        /// receive urgent pointer
        bool up{0};
        /// initial receive sequence number
        uint32_t irs{0};
    };

    struct SendSequenceSpace
    {
        /// send unacknowledged
        uint32_t una;
        /// send next
        uint32_t nxt;
        /// send window
        uint16_t wnd;
        /// send urgent pointer
        bool up;
        /// segment sequence number used for last window update
        size_t wl1;
        /// segment acknowledgment number used for last window update
        size_t wl2;
        /// initial send sequence number
        uint32_t iss;
    };

    bool wrapping_lt(uint32_t lhs, uint32_t rhs)
    {
        // From RFC1323:
        //     TCP determines if a data segment is "old" or "new" by testing
        //     whether its sequence number is within 2**31 bytes of the left edge
        //     of the window, and if it is not, discarding the data as "old".  To
        //     insure that new data is never mistakenly considered old and vice-
        //     versa, the left edge of the sender's window has to be at most
        //     2**31 away from the right edge of the receiver's window.
        return (lhs - rhs) > (1 << 31);
    }

    bool is_between_wrapped(uint32_t start, uint32_t x, uint32_t end)
    {
        return wrapping_lt(start, x) && wrapping_lt(x, end);
    }

    struct Timers
    {
        std::map<uint32_t, std::chrono::steady_clock> send_times;
        double srtt;
    };

    enum class State
    {
        // Listen,
        SynRcvd,
        Write,
        Estab,
        FinWait1,
        FinWait2,
        TimeWait
    };

    struct Connection
    {
    public:
        // Attributes
        State state;
        RecvSequenceSpace recv;
        SendSequenceSpace send;
        ip iph;
        tcphdr tcp;
        Timers timers;

        boost::circular_buffer<uint8_t> incoming;
        boost::circular_buffer<uint8_t> unacked;

        bool closed;
        std::optional<uint32_t> closed_at;

    public:
        // Methods
        Available availability()
        {
            // unimplemented();
            //  if (self.is_rcv_closed() || !self.incoming.is_empty()) {
            //      a |= Available::READ;
            //  }
            return tcp::Available::READ;
        }
        auto is_rcv_closed() -> bool
        {
            if (match(state, State::TimeWait))
            {
                // TODO: any state after rcvd FIN, so also CLOSE-WAIT, LAST-ACK, CLOSED, CLOSING
                return true;
            }
            else
            {
                return false;
            }
        }
        void on_tick(auto &tun)
        {

            if (match(state, State::FinWait2, State::TimeWait))
            {
                // we have shutdown our write side and the other side acked, no need to (re)transmit anything
                write(tun, send.una, 0);
                return;
            }

            // eprintln!("ON TICK: state {:?} una {} nxt {} unacked {:?}",
            //           self.state, self.send.una, self.send.nxt, self.unacked);

            uint32_t nunacked_data = ssub_tohs(unwrap_or(closed_at, send.nxt), send.una);
            uint32_t nunsent_data = unacked.size() - nunacked_data;

            // let waited_for = self
            //     .timers
            //     .send_times
            //     .range(self.send.una..)
            //     .next()
            //     .map(|t| t.1.elapsed());

            bool should_retransmit = 0;
            // bool should_retransmit = [] {
            //     if let Some(waited_for) = waited_for {
            //         waited_for > time::Duration::from_secs(1)
            //             && waited_for.as_secs_f64() > 1.5 * self.timers.srtt
            //     } else {
            //         return false;
            //     }
            // }();

            if (should_retransmit)
            {
                auto resend = std::min((uint32_t)unacked.size(), (uint32_t)send.wnd);
                if (resend < send.wnd && closed)
                {
                    // can we include the FIN?
                    tcp.fin = 1;
                    closed_at = add(send.una, unacked.size());
                }
                write(tun, send.una, resend);
            }
            else
            {
                switch (state)
                {
                case State::Write:
                    tcp.psh = 1;
                    write(tun, send.nxt, unacked.size());

                    state = State::Estab;
                    break;
                default:
                    break;
                }
            }

            // std::cout << "on tick" << std::endl;
        }

        size_t write(TunTap &tun, uint32_t seq, size_t limit)
        {
            std::vector<uint8_t> buf;
            buf.resize(1500);
            auto unwritten = std::span{buf};

            tcp.seq = seq;
            tcp.ack_seq = recv.nxt;

            // TODO: return +1 for SYN/FIN
            std::cout << std::format(
                "write(ack: {}, seq: {}, limit: {}) syn {} fin {}",
                recv.nxt - recv.irs, seq, limit, (int)tcp.syn, (int)tcp.fin);

            size_t offset = ssub_tohs(seq, send.una);
            // we need to special-case the two "virtual" bytes SYN and FIN

            if (closed_at.has_value() && seq == *closed_at + 1)
            {
                offset = 0;
                limit = 0;
            }
            std::cout << std::format(
                "using offset {} base {}\n",
                offset,
                send.una //,
                // self.unacked.as_slices()
            );

            auto [h, t] = as_slices(unacked);

            if (h.size() >= offset)
            {
                h = {h.begin() + offset, h.end()};
            }
            else
            {
                auto skipped = h.size();
                h = {};
                t = {t.begin() + offset - skipped, t.end()};
            }

            auto max_data = std::min(limit, h.size() + t.size());
            auto size = std::min(
                buf.size(),
                header_size(tcp) + header_size(iph) + max_data);
            iph.ip_len = htons(sizeof(ip) + sizeof(tcphdr) + max_data);
            iph.ip_sum = checksum(reinterpret_cast<uint16_t *>(&iph), iph.ip_hl << 2);

            // write out the headers and the payload
            const auto buf_len = buf.size();
            unwritten = unwritten.subspan(write_into_buffer(iph, unwritten));
            auto ip_header_ends_at = buf_len - unwritten.size();

            // postpone writing the tcp header because we need the payload as one contiguous slice to calculate the tcp checksum
            unwritten = unwritten.subspan(header_size(tcp));
            // write_into_buffer(tcp, unwritten); // a changer a caquse du checksum
            auto tcp_header_ends_at = buf_len - unwritten.size();

            // write out the payload
            auto payload_bytes = [&]
            {
                auto written = 0;
                auto limit = max_data;

                // // first, write as much as we can from h
                auto p1l = std::min(limit, h.size());
                PRINT_VAR(h.size());
                written += write_into_buffer(decltype(t){h.begin(), h.begin() + p1l}, unwritten);
                limit -= written;
                unwritten = unwritten.subspan(written);

                // // then, write more (if we can) from t
                // auto p2l = std::min(limit, t.size());
                // PRINT_VAR(t.size());
                // written += write_into_buffer(decltype(t){t.begin(), t.begin() + p2l}, unwritten);
                // unwritten = unwritten.subspan(written);
                return written;
            }();
            PRINT_VAR(payload_bytes);
            auto payload_ends_at = buf_len - unwritten.size();
            PRINT_VAR(payload_ends_at);

            // finally we can calculate the tcp checksum and write out the tcp header
            tcp.check = calc_tcp_checksum(iph, tcp, {buf.begin() + tcp_header_ends_at, buf.begin() + payload_ends_at});

            PRINT_VAR(tcp_header_ends_at);

            PRINT_VAR(ntohs(tcp.check));

            // tcp.checksum = tcp
            //     .calc_checksum_ipv4(&self.ip, &buf[tcp_header_ends_at..payload_ends_at])
            //     .expect("failed to compute checksum");
            std::memcpy(buf.data() + ip_header_ends_at, &tcp, tcp_header_ends_at - ip_header_ends_at);
            // self.tcp.write(&mut tcp_header_buf);
            //  auto tcp_header_buf = std::span{buf.begin() + ip_header_ends_at,
            //      buf.begin() + tcp_header_ends_at};
            // write_into_buffer(tcp, tcp_header_buf);

            PRINT_VAR(payload_bytes);
            uint32_t next_seq = add(seq, payload_bytes);
            PRINT_VAR(ntohl(next_seq));
            if (tcp.syn)
            {
                next_seq = add(next_seq, 1);
                tcp.syn = 0;
            }
            if (tcp.fin)
            {
                next_seq = add(next_seq, 1);
                // tcp.fin = 0;
            }
            if (wrapping_lt(ntohl(send.nxt), ntohl(next_seq)))
            {
                send.nxt = next_seq;
            }
            // timers.send_times[seq] = std::chrono::steady_clock::now(); does not compile

            tun.send(buf, payload_ends_at);
            // unimplemented();
            return 0;
        }

        std::optional<Available> on_packet(TunTap &ih,
                                           const ip *ipv4h,
                                           const tcphdr *tcph,
                                           const std::vector<char> &data)
        {

            print_buffer(data);
            uint32_t seqn = tcph->seq;
            PRINT_VAR(ntohl(seqn));
            size_t slen = data.size();

            if (tcph->fin)
            {
                slen += 1;
            }
            if (tcph->syn)
            {
                slen += 1;
            }

            std::cout << "0" << ntohl(recv.nxt) << " " << ntohs(recv.wnd) << std::endl;

            uint32_t wend = add(recv.nxt, ntohs(recv.wnd));

            // Todo : Simplify this
#ifdef NOT_OKAY_DEBUG
            std::cout << "1)" << std::format("{} {} {}", sub_tohs(recv.nxt, 1), ntohl(seqn), ntohl(wend)) << std::endl;
            std::cout << "2)" << std::format("{}", add_tohs(seqn, slen - 1)) << std::endl;
#endif
            auto okay = [&]() -> bool
            {
                if (slen == 0)
                {
                    if (recv.wnd == 0)
                    {
                        return seqn == recv.nxt;
                    }
                    else if (!is_between_wrapped(sub_tohs(recv.nxt, 1), ntohl(seqn), ntohl(wend)))
                    {
                        return false;
                    }
                    else
                    {
                        return true;
                    }
                }
                else
                {
                    if (recv.wnd == 0)
                    {
                        return false;
                    }
                    else if (!is_between_wrapped(sub_tohs(recv.nxt, 1), ntohl(seqn), ntohs(wend)) &&
                             !is_between_wrapped(sub_tohs(recv.nxt, 1), add_tohs(seqn, slen - 1), ntohl(wend)))
                    {
                        return false;
                    }
                    else
                    {
                        return true;
                    }
                }
            }();
            if (!okay)
            {
                std::cerr << "Not okay\n";
                write(ih, send.nxt, 0);
                return availability();
            }

            if (!tcph->ack)
            {
                if (tcph->syn)
                {
                    // got SYN part of initial handshake
                    assert(data.empty());
                    recv.nxt = add(seqn, 1);
                }
                std::cout << "early ret\n";
                return availability();
            }

            auto ackn = tcph->ack_seq;
            PRINT_VAR(ntohl(ackn));
            if (State::SynRcvd == state)
            {
                if (is_between_wrapped(
                        sub_tohs(send.una, 1),
                        ntohl(ackn),
                        add_tohs(send.nxt, 1)))
                {
                    // must have ACKed our SYN, since we detected at least one acked byte,
                    // and we have only sent one byte (the SYN).
                    state = State::Estab;
                }
                else
                {
                    // TODO: <SEQ=SEG.ACK><CTL=RST>
                }
            }

            if (match(state, State::Estab, State::FinWait1, State::FinWait2))
            {
                if (is_between_wrapped(ntohl(send.una), ntohl(ackn), add_tohs(send.nxt, 1)))
                {
                    std::cout << std::format(
                        "ack for {} (last: {})\n",
                        ntohl(ackn), ntohl(send.una));
                    if (!unacked.empty())
                    {
                        auto data_start = [&]
                        {
                            // send.una hasn't been updated yet with ACK for our SYN, so data starts just beyond it
                            if (send.una == send.iss)
                                return add(send.una, 1);
                            return send.una;
                        }();
                        auto acked_data_end = std::min((size_t)ssub_tohs(ackn, data_start), unacked.size());

                        unacked.erase(unacked.begin(), unacked.begin() + acked_data_end);
                        // Todo : Future impl, Improve replace and copy
                        auto old = replace(timers.send_times, {});

                        auto una = send.una;
                        auto srtt = timers.srtt;

                        std::copy_if(old.begin(),
                                     old.end(),
                                     std::inserter(timers.send_times, timers.send_times.end()),
                                     [&](auto it)
                                     {
                                         auto &[seq, sent] = it;
                                         if (is_between_wrapped(ntohl(una), ntohl(seq), ntohl(ackn)))
                                         {
                                             //*srtt = 0.8 * *srtt + (1.0 - 0.8) * sent.elapsed().as_secs_f64();
                                             return false;
                                         }
                                         else
                                         {
                                             return true;
                                         }
                                     });
                    }

                    send.una = ackn;
                }

                // TODO: if unacked empty and waiting flush, notify
                // TODO: update window
            }

            if (state == State::FinWait1)
            {
                if (closed_at.has_value())
                {
                    if (send.una == add(*closed_at, 1))
                    {
                        // our FIN has been ACKed!
                        state = State::FinWait2;
                    }
                    else
                    {
                        std::cout << std::format("una({}) diff de closed_at ({}) + 1", send.una, *closed_at) << std::endl;
                    }
                }
            }

            if (!data.empty())
            {
                std::cout << "Recv with data: " << data.size() << std::endl;
                if (match(state, State::Estab, State::FinWait1, State::FinWait2))
                {
                    FOCUS();
                    size_t unread_data_at = ssub_tohs(recv.nxt, seqn);
                    if (unread_data_at > data.size())
                    {
                        // we must have received a re-transmitted FIN that we have already seen
                        // nxt points to beyond the fin, but the fin is not in data!
                        std::cout << std::format("unread_data_at: {} size: {}\n",
                                                 unread_data_at,
                                                 data.size());
                        // assert(unread_data_at == data.size() + 1 && "unread_data_at != data.len() + 1");
                        unread_data_at = 0;
                    }
                    std::cout << "unread_data_at" << unread_data_at << std::endl;
                    std::cout << "DATA " << std::endl;

                    for (auto d : data)
                    {
                        std::cout << d << " ";
                        incoming.push_back(d);
                    }
                    std::cout << std::endl;
                    // std::copy(data.begin() + unread_data_at, data.end(),
                    //     std::back_inserter(incoming));
                    std::cout << &incoming << std::endl;
                    std::cout << "insterted" << incoming.size() << std::endl;
                    /*
                    Once the TCP takes responsibility for the data it advances
                    RCV.NXT over the data accepted, and adjusts RCV.WND as
                    apporopriate to the current buffer availability.  The total of
                    RCV.NXT and RCV.WND should not be reduced.
                     */
                    recv.nxt = add(seqn, (uint32_t)data.size());

                    // Send an acknowledgment of the form: <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK>
                    // TODO: maybe just tick to piggyback ack on data?
                    write(ih, send.nxt, 0);
                }
            }

            if (tcph->fin)
            {
                if (state == State::FinWait2 || true)
                {
                    tcp.fin = 1;
                    // TODO : Run a GDP and try to understand why we are dying !
                    FOCUS();
                    // Let's see how it works without calling shutdown !
                    // we're done with the connection!
                    recv.nxt = add(recv.nxt, 1);
                    write(ih, send.nxt, 0);
                    state = State::TimeWait;
                }
                else
                {
                    std::cout << std::format("unknown state {}", (int)state) << std::endl;
                    unimplemented();
                }
            }
            return availability();
        }
        static std::optional<Connection> accept(TunTap &tun, ip ipv4h, tcphdr tcph, const std::vector<char> &buff)
        {
            if (!tcph.syn)
            {
                return std::nullopt;
            }

            uint16_t iss = 0;
            uint16_t wnd = htons(1024);

            // std::swap(ipv4h.ip_src, ipv4h.ip_dst);

            tcphdr tcp;
            std::memset(&tcp, 0, sizeof(tcp));

            tcp.doff = 5; // TODO : Add a define, 5 is the min len of tcp header -> 20 bytes
            tcp.th_dport = tcph.th_sport;
            tcp.th_sport = tcph.th_dport;

            ip tmp_iph = make_iph(0, 64, IPPROTO_TCP, ipv4h.ip_dst, ipv4h.ip_src);

            tcp.window = wnd;

            auto c = Connection{
                .state{
                    State::SynRcvd},
                .recv{
                    .nxt{add(tcph.seq, 1)},
                    .wnd{tcph.window},
                    .up{0},
                    .irs{tcph.seq},
                },
                .send{
                    .una{iss},
                    .nxt{iss},
                    .wnd{wnd},
                    .up{0},
                    .wl1{0},
                    .wl2{0},
                    .iss{iss},
                },
                .iph{
                    tmp_iph},
                .tcp{
                    tcp},
                .timers{
                    .send_times{},
                    .srtt{std::chrono::duration<double>(std::chrono::seconds{1 * 60}).count()}},
                .incoming{boost::circular_buffer<uint8_t>(1024)},
                .unacked{
                    boost::circular_buffer<uint8_t>(1024)},
                .closed{
                    false},
                .closed_at{
                    std::nullopt}};

            // // need to start establishing a connection
            c.tcp.syn = 1;
            c.tcp.ack = 1;
            // c.tcp.check = ntohs(0x8d4d); // TODO: a calculer
            c.write(tun, c.send.nxt, 0);
            return c;
        }
    };

}