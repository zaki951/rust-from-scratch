# Lightweight TCP Implementation in C++

This project is a lightweight TCP implementation in C++, inspired by the educational Rust project [jonhoo/rust-tcp](https://github.com/jonhoo/rust-tcp).

## Overview

A simple TCP implementation in C++ for learning and exploring network programming concepts.

## Features

- Basic TCP connection handling
- Simple send/receive messaging
- Minimal dependencies

## Usage

### Terminal 1

Run the program (you might need `sudo` to create a TUN interface):

```bash
./run.sh
```

You can verify the TUN interface with ifconfig (example):

```
tun0: flags=4305<UP,POINTOPOINT,RUNNING,NOARP,MULTICAST>  mtu 1500
    inet 10.0.0.1  netmask 255.255.255.0  destination 10.0.0.1
```

### Terminal 2
Connect using netcat: `nc 10.0.0.2 8000`

Netcat should receive: `Hello from cpp!`

You can reply with any message, which will be received by the first terminal.

### Notes
* At this stage, the FIN packet does not seem to be sent properly, netcat may stay connected.
* The TUN interface is deleted properly when the program ends.
* On WSL, sometimes the terminal behaves oddly after running the TCP program (e.g., input issues).

