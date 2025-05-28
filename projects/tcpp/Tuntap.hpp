#pragma once

#include <stdexcept>
#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

extern "C"
{
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <fcntl.h>
#include <linux/if_tun.h>
#include <unistd.h>
}

#include "Helper.hpp"

#define TUNSETIFF _IOW('T', 202, int)

class TunTap
{
public:
	static constexpr char PATH_TUNTAP[] = "/dev/net/tun";
	TunTap() : _fd{open(PATH_TUNTAP, O_RDWR)}
	{
		if (_fd == -1)
			throw std::runtime_error("Tun/Tap cannont be initialized");

		struct ifreq ifr;

		memset(&ifr, 0, sizeof(ifr));
		ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

		strncpy(ifr.ifr_name, "tun0", IFNAMSIZ);

		if (ioctl(_fd, TUNSETIFF, &ifr) == -1)
			throw std::runtime_error("Tun/Tap cannot be set");

		int ioresult = ioctl(_fd, TUNSETIFF, &ifr);
		// if (ioresult < 0)
		// 	return ioresult;

		// if (non_blocking)
		// 	fcntl(_fd, F_SETFL, O_NONBLOCK);
	}

	int recv(std::vector<char> &v)
	{
		if (_fd == -1)
			throw std::runtime_error("Invalid File Descriptor");

		if (v.size() < 1)
			throw std::runtime_error("The buffer should have a size greater than 0");

		return read(_fd, &v[0], v.size());
	}

	int send(std::vector<uint8_t> &v, int end = 0)
	{
		print_buffer(v, end);
		if (end == 0)
			end = v.size();
		if (_fd == -1)
			throw std::runtime_error("Invalid File Descriptor");

		if (v.size() < 1)
			throw std::runtime_error("The buffer should have a size greater than 0");

		return write(_fd, &v[0], end);
	}
	int get_fd()
	{
		return _fd;
	}

	~TunTap()
	{
		if (_fd != -1)
		{
			close(_fd);
		}
	}

private:
	int _fd{-1};
};
