#pragma once
#include "protocol.h"
#include "spsc_queue.h"
#include <string>
#include <atomic>

namespace hft::feed {

// We define a wrapper struct for the queue that holds the message AND its kernel receipt time (t1)
struct QueueItem {
    OrderMessage msg;
    uint64_t kernel_t1_ns;
};

// Aliasing the queue to size 1024 (must be power of 2)
using FeedQueue = SPSCQueue<QueueItem, 1024>;

class FeedHandler {
private:
    int socket_fd_{-1};
    FeedQueue& queue_;
    std::atomic<bool>& running_;

public:
    FeedHandler(uint16_t port, FeedQueue& queue, std::atomic<bool>& running_flag);
    ~FeedHandler();

    // The hot loop: busy-polls the socket, reads packets, extracts t1, and pushes to SPSC
    void listen();
};

} // namespace hft::feed
