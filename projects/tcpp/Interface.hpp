#pragma once

#include <thread>
#include <memory>
#include <format>
#include <unordered_map>
#include <string>
#include <condition_variable>
#include <optional>

extern "C"
{
#include <poll.h>
}
#include <cstdint>
#include "tcp.hpp"
#include "Interface.hpp"
#include "Helper.hpp"
#include "IpParse.hpp"
#include "Safe.hpp"

struct Quad
{
    std::pair<in_addr, uint16_t> src;
    std::pair<in_addr, uint16_t> dst;

    bool operator==(const Quad &q) const
    {
        if (q.src.first.s_addr == src.first.s_addr &&
            q.dst.first.s_addr == dst.first.s_addr &&
            q.src.second == src.second && q.dst.second == dst.second)
        {
            return true;
        }

        return false;
    }
};

std::ostream &operator<<(std::ostream &out, const Quad &q)
{
    out << std::format("src: ({},{})\n", inet_ntoa(q.src.first), q.src.second);
    out << std::format("dst: ({},{})\n", inet_ntoa(q.dst.first), q.dst.second);
    return out;
}

class QuadHash
{
public:
    size_t operator()(const Quad &p) const
    {
        size_t h1 = std::hash<uint32_t>()(p.src.first.s_addr);
        size_t h2 = std::hash<uint32_t>()(p.dst.first.s_addr);
        size_t h3 = std::hash<uint16_t>()(p.src.second);
        size_t h4 = std::hash<uint16_t>()(p.dst.second);

        return h1 ^ h2 ^ h3 ^ h4;
    }
};

struct ConnectionManager
{
    bool termninate;
    std::unordered_map<Quad, tcp::Connection, QuadHash> connections;
    std::unordered_map<uint16_t, std::deque<Quad>> pending;
};

struct InterfaceHande_s
{
    Safe<ConnectionManager> manager;
    std::condition_variable pending_var;
    std::condition_variable recv_var;
};

class PollFd
{
public:
    PollFd(int fd)
    {
        int flags = fcntl(fd, F_GETFL);
        if (fd == -1)
        {
            throw std::runtime_error("invalid file descriptor");
        }
        _pfd.fd = fd;
        _pfd.events = POLLIN;
    }
    int pollx(int time)
    {
        int n = poll(&_pfd, 1, time);
        if (n < 0)
        {
            // Don't need to close _fd, will be closed by TunTap destructor
            throw std::runtime_error("Poll fail");
        }
        return n;
    }

private:
    struct pollfd _pfd;
};

struct TcpStream
{
    Quad quad;
    std::shared_ptr<InterfaceHande_s> ih;

    size_t read(std::vector<char> &buf)
    {
        auto [cm, lock] = ih->manager.lock();
        while (true)
        {

            if (cm.connections.find(quad) == cm.connections.end())
            {
                std::cout << quad << std::endl;
                // Todo : Fix the issue in accept
                throw "Quad not found !";
            }
            auto &c = cm.connections[quad];

            if (c.is_rcv_closed() && c.incoming.empty())
            {
                // no more data to read, and no need to block, because there won't be any more
                return 0;
            }
            std::cout << &c.incoming << std::endl;
            if (!c.incoming.empty())
            {

                std::copy(c.incoming.begin(), c.incoming.end(), std::back_inserter(buf));
                int nread = c.incoming.size();
                c.incoming.clear();
                return nread;
            }

            ih->recv_var.wait(lock);
        }
    }

    size_t write(const std::vector<uint8_t> &buf)
    {
        std::cout << "Write buffer: " << (const char *)buf.data() << std::endl;
        FOCUS(); // Implement a way to send all the buffer
        auto [cm, lock] = ih->manager.lock();
        if (cm.connections.find(quad) == cm.connections.end())
        {
            std::cout << quad << std::endl;
            throw "Quad not found !";
        }
        auto &c = cm.connections[quad];
        c.state = tcp::State::Write;
        assert(buf.size() > c.unacked.size());
        std::copy(buf.begin(), buf.end(), std::back_inserter(c.unacked));
        PRINT_VAR(buf.size());
        return buf.size();
    }
};

struct TcpListener
{
    uint16_t port;
    std::shared_ptr<InterfaceHande_s> ih;

    std::optional<TcpStream> accept()
    {
        auto [cm1, lock] = ih->manager.lock();

        while (true)
        {
            bool found = cm1.pending.find(port) != cm1.pending.end();
            if (found && !cm1.pending[port].empty())
            {
                auto front = cm1.pending[port].front();
                auto ret = TcpStream{
                    .quad = front,
                    .ih = ih};
                cm1.pending[port].pop_front();
                return ret;
            }
            ih->pending_var.wait(lock);
        }
        return std::nullopt;
    }
};

class Interface
{

public:
    Interface()
    {
        _ih = std::make_shared<InterfaceHande_s>(); //  does not compile
        _t = std::thread{&Interface::packet_loop, this, _ih};
    }
    ~Interface()
    {
        _end = true;

        if (_t.joinable())
            _t.join();
    }
    std::optional<TcpListener> bind(uint16_t port)
    {
        auto [cm, _] = _ih->manager.lock();
        auto it = cm.pending.find(port);
        if (it != cm.pending.end())
        {
            std::cerr << "port already bound\n";
            return std::nullopt;
        }
        else
        {
            cm.pending[port] = {};
        }

        return TcpListener{
            .port = port,
            .ih = _ih};
    }

private:
    // State : 90%. Usable state : OK
    void packet_loop(std::shared_ptr<InterfaceHande_s> ih)
    {
        TunTap tun;
        std::vector<char> buff;
        buff.resize(1024);
        while (!_end)
        {
            PollFd pd{tun.get_fd()};
            int n = pd.pollx(10);
            if (n == 0)
            {
                /* timeout */
                auto [cmg, _] = _ih->manager.lock();
                for (auto &[_, connection] : cmg.connections)
                {
                    // XXX: don't die on errors?
                    connection.on_tick(tun);
                }
                continue;
            }
            assert(n == 1);
            int nbytes = tun.recv(buff);

            if (nbytes > 0)
            {
                const ip *ipv4h = parse_ipv4_packet(buff);
                if (!ipv4h)
                    continue;

                if (ipv4h->ip_p != 6)
                {
                    std::cerr << "The protocol is not TCP: " << (int)ipv4h->ip_p << "\n";
                    continue;
                }
                const tcphdr *tcph = parse_tcp_packet(buff);
                if (!tcph)
                {
                    std::cerr << "Invalid TCP header\n";
                    continue;
                }

                // size_t datai = sizeof(ipv4h) + sizeof(tcph);
                size_t datai = header_size(*ipv4h) + header_size(*tcph);
                auto [cm, _] = ih->manager.lock();
                auto q = Quad{
                    .src = {
                        ipv4h->ip_src,
                        ntohs(tcph->th_sport)},
                    .dst = {ipv4h->ip_dst, ntohs(tcph->th_dport)}};
                auto pair = cm.connections.find(q);
                decltype(buff) payload{buff.begin() + datai, buff.begin() + nbytes};
                if (pair != cm.connections.end())
                {
                    auto &[_, c] = *pair;
                    std::cout << "Got packet for known quad "
                              << q
                              << " payload size: "
                              << payload.size()
                              << " datai: "
                              << datai
                              << " nbytes: "
                              << nbytes
                              << std::endl;
                    // Call tcp on_packet
                    auto state = c.on_packet(tun, ipv4h, tcph, payload);
                    if (state == tcp::Available::READ)
                    {
                        ih->recv_var.notify_all();
                    }
                    if (state == tcp::Available::WRITE)
                    {
                        unimplemented();
                    }
                }
                else
                {
                    std::cout << "Got packet for unknown quad " << q << std::endl;

                    auto pair = cm.pending.find(ntohs(tcph->th_dport));
                    if (pair != cm.pending.end())
                    {
                        auto &[_, pending] = *pair;
                        auto c = tcp::Connection::accept(tun, *ipv4h, *tcph, payload);
                        if (c.has_value())
                        {
                            cm.connections[q] = *c;
                        }
                        pending.push_back(q);
                        ih->pending_var.notify_all();
                    }
                }
            }
        }
    }
    std::atomic<bool> _end{false};
    std::thread _t;
    std::shared_ptr<InterfaceHande_s> _ih;
};
