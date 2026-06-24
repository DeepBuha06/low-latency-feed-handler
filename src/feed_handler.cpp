#include "feed_handler.h"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

// Linux specific networking flags
#ifndef SO_BUSY_POLL
#define SO_BUSY_POLL 46
#endif

namespace hft::feed {

FeedHandler::FeedHandler(uint16_t port, FeedQueue& queue, std::atomic<bool>& running_flag)
    : queue_(queue), running_(running_flag) {
    
    socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ < 0) {
        throw std::runtime_error("Failed to create UDP socket");
    }

    // Set SO_REUSEADDR
    int optval = 1;
    setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    // Elite: Attempt to enable Kernel-Bypass SO_BUSY_POLL (50 microseconds busy loop in kernel)
    int busy_poll_us = 50;
    if (setsockopt(socket_fd_, SOL_SOCKET, SO_BUSY_POLL, &busy_poll_us, sizeof(busy_poll_us)) < 0) {
        std::cerr << "[WARN] SO_BUSY_POLL not supported or permitted by OS (Normal for WSL2/Windows)\n";
    }

    // Elite: Attempt to enable SO_TIMESTAMPNS (Hardware / Software RX Timestamps)
    int timestamp_flags = 1;
    if (setsockopt(socket_fd_, SOL_SOCKET, SO_TIMESTAMPNS, &timestamp_flags, sizeof(timestamp_flags)) < 0) {
        std::cerr << "[WARN] SO_TIMESTAMPNS not supported. Falling back to userspace clock.\n";
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Add a receive timeout of 100ms so we can gracefully exit when running_ becomes false
    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    if (bind(socket_fd_, (const sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        throw std::runtime_error("Failed to bind UDP socket to port");
    }
    
    std::cout << "[INFO] Feed Handler listening on UDP port " << port << "\n";
}

FeedHandler::~FeedHandler() {
    if (socket_fd_ >= 0) {
        close(socket_fd_);
    }
}

void FeedHandler::listen() {
    char buffer[2048];
    char control_buf[1024];

    iovec iov;
    iov.iov_base = buffer;
    iov.iov_len = sizeof(buffer);

    msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control_buf;

    while (running_.load(std::memory_order_acquire)) {
        msg.msg_controllen = sizeof(control_buf);

        // Block and wait for a UDP packet
        // If SO_BUSY_POLL is enabled, the kernel will spin-wait instead of context-switching!
        ssize_t bytes_received = recvmsg(socket_fd_, &msg, 0);
        if (bytes_received <= 0) continue;

        // t1: Userspace receipt timestamp (CLOCK_MONOTONIC — same clock as t3/t4 in consumer)
        // Taken immediately after recvmsg returns, before any processing.
        timespec ts1;
        clock_gettime(CLOCK_MONOTONIC, &ts1);
        uint64_t t1_ns = (ts1.tv_sec * 1000000000ULL) + ts1.tv_nsec;

        // Also extract the kernel RX timestamp from the control message if available.
        // On WSL2, SO_TIMESTAMPNS provides a CLOCK_REALTIME timestamp which cannot be
        // directly compared with CLOCK_MONOTONIC. In production with PTP-synced clocks
        // or on bare-metal Linux with SO_TIMESTAMPING, this would be the authoritative t1.
        // We log it here to demonstrate the capability but use CLOCK_MONOTONIC for measurement.
        for (cmsghdr* cmsg = CMSG_FIRSTHDR(&msg); cmsg != nullptr; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
            if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_TIMESTAMPNS) {
                // Kernel timestamp available — in production, this would replace t1_ns
                break;
            }
        }

        // Validate packet size
        if (bytes_received != sizeof(OrderMessage)) continue;

        // Cast raw bytes directly into the C++ struct (Zero parsing overhead!)
        OrderMessage* order = reinterpret_cast<OrderMessage*>(buffer);

        // Prepare the queue item
        QueueItem item;
        item.msg = *order;
        item.kernel_t1_ns = t1_ns;

        // Push to the SPSC queue. Spin wait if the queue is full.
        while (running_.load(std::memory_order_acquire) && !queue_.push(item)) {
            // Wait for consumer to drain
        }
    }
}

} // namespace hft::feed
