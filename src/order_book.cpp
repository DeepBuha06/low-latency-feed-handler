#include "order_book.h"

namespace hft::feed {

void OrderBook::apply(const OrderMessage& msg) {
    // Bounds check to prevent buffer overflow (crucial in C++)
    // as the price_ticks is a uint32_t if the tick underflows, we have a very massive number 
    // so we can just skip underflow comparison
    if (msg.price_ticks >= LOB_ARRAY_SIZE) {
        return; // Drop invalid price gracefully
    }

    auto& book = (msg.side == Side::BID) ? bids_ : asks_;

    if (msg.action == OrderAction::ADD) {
        book[msg.price_ticks] += msg.quantity;
    } else if (msg.action == OrderAction::CANCEL) {
        if (book[msg.price_ticks] >= msg.quantity) {
            book[msg.price_ticks] -= msg.quantity;
        } else {
            book[msg.price_ticks] = 0; // Prevent unsigned underflow
        }
    } else if (msg.action == OrderAction::MODIFY) {
        book[msg.price_ticks] = msg.quantity; // Simplified modify logic for prototype
    }
}

uint32_t OrderBook::get_volume_at_price(Side side, uint32_t price_ticks) const {
    if (price_ticks >= LOB_ARRAY_SIZE) return 0;
    return (side == Side::BID) ? bids_[price_ticks] : asks_[price_ticks];
}

} // namespace hft::feed
