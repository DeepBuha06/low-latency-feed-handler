#pragma once
#include "protocol.h"
#include <array>
#include <cstdint>

namespace hft::feed {

class OrderBook {
private:
    // O(1) Flat Arrays for price levels
    // The index represents the price offset from MIN_PRICE in TICK_SIZE increments.
    std::array<uint32_t, LOB_ARRAY_SIZE> bids_{0};
    std::array<uint32_t, LOB_ARRAY_SIZE> asks_{0};

public:
    OrderBook() = default;

    // Apply an incoming network order to the book
    void apply(const OrderMessage& msg);
    
    // For verification and testing
    uint32_t get_volume_at_price(Side side, uint32_t price_ticks) const;
};

} // namespace hft::feed
