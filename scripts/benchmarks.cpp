#include <iostream>
#include <vector>
#include <time.h>
#include <algorithm>
#include <numeric>
#include <thread>
#include <atomic>
#include "../include/order_book.h"
#include "../include/protocol.h"
#include "../third_party/spsc_queue/include/spsc_queue.h"

using namespace hft::feed;

uint64_t get_ns(timespec& ts) {
    return (ts.tv_sec * 1000000000ULL) + ts.tv_nsec;
}

void pin_thread(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

void bench_clock() {
    const int N = 1000000;
    std::vector<uint64_t> diffs(N);
    timespec t1, t2;
    for(int i=0; i<N; ++i) {
        clock_gettime(CLOCK_MONOTONIC, &t1);
        clock_gettime(CLOCK_MONOTONIC, &t2);
        diffs[i] = get_ns(t2) - get_ns(t1);
    }
    std::sort(diffs.begin(), diffs.end());
    std::cout << "Clock Overhead (back-to-back):\n";
    std::cout << "  p50:   " << diffs[N/2] << " ns\n";
    std::cout << "  p99:   " << diffs[N*99/100] << " ns\n";
    std::cout << "  p99.9: " << diffs[N*999/1000] << " ns\n\n";
}

void bench_book() {
    const int N = 1000000;
    std::vector<uint64_t> diffs(N);
    OrderBook book;
    OrderMessage msg;
    msg.price_ticks = 100;
    msg.quantity = 10;
    msg.side = Side::BID;
    msg.action = OrderAction::ADD;
    
    timespec t1, t2;
    // Warmup
    for(int i=0; i<1000; ++i) book.apply(msg);

    for(int i=0; i<N; ++i) {
        msg.price_ticks = i % 2000; // vary to prevent perfect caching, though still L1
        clock_gettime(CLOCK_MONOTONIC, &t1);
        book.apply(msg);
        clock_gettime(CLOCK_MONOTONIC, &t2);
        diffs[i] = get_ns(t2) - get_ns(t1);
    }
    std::sort(diffs.begin(), diffs.end());
    std::cout << "Book Apply (isolated):\n";
    std::cout << "  p50:   " << diffs[N/2] << " ns\n";
    std::cout << "  p99:   " << diffs[N*99/100] << " ns\n";
    std::cout << "  p99.9: " << diffs[N*999/1000] << " ns\n\n";
}

struct DummyItem {
    OrderMessage msg;
    uint64_t t1;
};

void bench_queue() {
    const int N = 1000000;
    SPSCQueue<DummyItem, 1024> queue;
    std::vector<uint64_t> transit(N);
    std::atomic<bool> start{false};
    std::atomic<int> received{0};

    std::thread consumer([&]() {
        pin_thread(2);
        DummyItem item;
        while(received < N) {
            if(queue.pop(item)) {
                timespec t2;
                clock_gettime(CLOCK_MONOTONIC, &t2);
                transit[received] = get_ns(t2) - item.t1;
                received++;
            }
        }
    });

    std::thread producer([&]() {
        pin_thread(1);
        DummyItem item;
        item.msg.price_ticks = 50;
        for(int i=0; i<N; ++i) {
            timespec t1;
            clock_gettime(CLOCK_MONOTONIC, &t1);
            item.t1 = get_ns(t1);
            while(!queue.push(item)); // spin
            // Small backoff to let consumer pop, mimicking network arrival
            for(volatile int j=0; j<50; ++j);
        }
    });

    producer.join();
    consumer.join();

    std::sort(transit.begin(), transit.end());
    std::cout << "Queue Push+Pop Transit (cross-core):\n";
    std::cout << "  p50:   " << transit[N/2] << " ns\n";
    std::cout << "  p99:   " << transit[N*99/100] << " ns\n";
    std::cout << "  p99.9: " << transit[N*999/1000] << " ns\n\n";
}

void bench_empty_pipeline() {
    const int N = 1000000;
    SPSCQueue<DummyItem, 1024> queue;
    std::vector<uint64_t> transit(N);
    std::atomic<int> received{0};

    std::thread consumer([&]() {
        pin_thread(2);
        DummyItem item;
        while(received < N) {
            if(queue.pop(item)) {
                // mock book.apply removed!
                timespec t2;
                clock_gettime(CLOCK_MONOTONIC, &t2);
                transit[received] = get_ns(t2) - item.t1;
                received++;
            }
        }
    });

    std::thread producer([&]() {
        pin_thread(1);
        DummyItem item;
        for(int i=0; i<N; ++i) {
            // Mock recvmsg
            timespec t1;
            clock_gettime(CLOCK_MONOTONIC, &t1);
            item.t1 = get_ns(t1);
            while(!queue.push(item));
            for(volatile int j=0; j<100; ++j);
        }
    });

    producer.join();
    consumer.join();

    std::sort(transit.begin(), transit.end());
    std::cout << "Empty Pipeline (mock recvmsg -> push -> pop -> timestamp):\n";
    std::cout << "  p50:   " << transit[N/2] << " ns\n";
    std::cout << "  p99:   " << transit[N*99/100] << " ns\n";
    std::cout << "  p99.9: " << transit[N*999/1000] << " ns\n\n";
}

int main() {
    pin_thread(0);
    bench_clock();
    bench_book();
    bench_queue();
    bench_empty_pipeline();
    return 0;
}
