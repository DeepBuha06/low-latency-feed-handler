#pragma once
#include <cstdint>
#include <cstddef>

namespace hft::feed {

// Constant boundaries for the O(1) Limit Order Book
// Assuming a stock trading between $90.00 and $110.00 with 1-cent ticks.
// Array size = (110.00 - 90.00) / 0.01 = 2000 total slots
constexpr double MIN_PRICE = 90.00;
constexpr double MAX_PRICE = 110.00;
constexpr double TICK_SIZE = 0.01;
constexpr size_t LOB_ARRAY_SIZE = 2000;

// Enums for message typing
enum class OrderAction : uint8_t {
    ADD = 1,
    MODIFY = 2,
    CANCEL = 3
};

enum class Side : uint8_t {
    BID = 1,
    ASK = 2
};

// Packed structure to prevent compiler padding.
// This simulates a raw binary network protocol (like NASDAQ ITCH).
#pragma pack(push, 1)
struct OrderMessage {
    uint32_t sequence_number; // Used to detect UDP packet loss
    uint64_t timestamp_ns;    // Sender's timestamp (optional for this specific analysis)
    uint64_t order_id;
    uint32_t symbol_id;
    OrderAction action;
    Side side;
    uint32_t price_ticks;     // Price expressed as an integer multiplier of TICK_SIZE above MIN_PRICE
    uint32_t quantity;
};
#pragma pack(pop)

} // namespace hft::feed
