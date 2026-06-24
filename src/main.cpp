#include "feed_handler.h"
#include "order_book.h"
#include <iostream>
#include <thread>
#include <vector>
#include <csignal>
#include <fstream>
#include <pthread.h>
#include <time.h>

using namespace hft::feed;

std::atomic<bool> g_running{true};

void signal_handler(int) {
    std::cout << "\n[INFO] Caught SIGINT. Initiating graceful shutdown...\n";
    g_running.store(false, std::memory_order_release);
}

// Helper to pin thread to a CPU core
void pin_thread_to_core(pthread_t thread, int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    if (pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset) != 0) {
        std::cerr << "[WARN] Failed to pin thread to core " << core_id << ". Proceeding without affinity.\n";
    }
}

// Telemetry structure
struct LatencyRecord {
    uint64_t seq_num;
    uint64_t queue_transit_ns;
    uint64_t book_update_ns;
    uint64_t wire_to_book_ns;
};

int main() {
    std::signal(SIGINT, signal_handler);

    // Pre-allocate vector for telemetry to avoid runtime allocation overhead (Zero Allocation Principle)
    std::vector<LatencyRecord> telemetry;
    telemetry.reserve(5000000); 

    FeedQueue queue;
    OrderBook book;

    // Start Receiver Thread
    std::thread receiver_thread([&]() {
        pin_thread_to_core(pthread_self(), 1); // Pin to core 1
        try {
            FeedHandler handler(12345, queue, g_running);
            handler.listen();
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Receiver: " << e.what() << "\n";
            g_running.store(false, std::memory_order_release);
        }
    });

    // Start Consumer/Processor Thread
    std::thread processor_thread([&]() {
        pin_thread_to_core(pthread_self(), 2); // Pin to core 2
        
        uint32_t expected_seq = 1;
        QueueItem item;

        while (g_running.load(std::memory_order_acquire) || !queue.empty()) {
            if (queue.pop(item)) {
                // t3: Immediately after pop
                timespec ts3;
                clock_gettime(CLOCK_MONOTONIC, &ts3);
                uint64_t t3_ns = (ts3.tv_sec * 1000000000ULL) + ts3.tv_nsec;

                // Gap detection
                if (item.msg.sequence_number > expected_seq) {
                    std::cerr << "[WARN] Packet Loss Detected! Expected " << expected_seq 
                              << " but got " << item.msg.sequence_number << "\n";
                }
                expected_seq = item.msg.sequence_number + 1;

                // Book update
                book.apply(item.msg);

                // t4: Immediately after book update
                timespec ts4;
                clock_gettime(CLOCK_MONOTONIC, &ts4);
                uint64_t t4_ns = (ts4.tv_sec * 1000000000ULL) + ts4.tv_nsec;

                // Calculations — all timestamps use CLOCK_MONOTONIC (same clock source)
                uint64_t wire_to_book = t4_ns - item.kernel_t1_ns;
                uint64_t book_update = t4_ns - t3_ns;
                
                // Store telemetry
                if (telemetry.size() < telemetry.capacity()) {
                    telemetry.push_back({
                        item.msg.sequence_number,
                        0, // transit calculated in full model
                        book_update,
                        wire_to_book
                    });
                }
            }
        }
    });

    receiver_thread.join();
    processor_thread.join();

    std::cout << "[INFO] System shut down. Flushing " << telemetry.size() << " latency records to disk...\n";

    std::ofstream out("latency_telemetry.csv");
    out << "SeqNum,BookUpdate_ns,WireToBook_ns\n";
    for (const auto& r : telemetry) {
        out << r.seq_num << "," << r.book_update_ns << "," << r.wire_to_book_ns << "\n";
    }

    std::cout << "[INFO] Telemetry saved to latency_telemetry.csv. Ready for plot_histogram.py.\n";

    return 0;
}
