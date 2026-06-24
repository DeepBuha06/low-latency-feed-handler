#include <iostream>
#include <cassert>
#include "../include/order_book.h"
#include "../include/protocol.h"

using namespace hft::feed;

void test_add_order() {
    OrderBook book;
    OrderMessage msg;
    msg.sequence_number = 1;
    msg.price_ticks = 1000;
    msg.quantity = 50;
    msg.side = Side::BID;
    msg.action = OrderAction::ADD;

    book.apply(msg);
    assert(book.get_volume_at_price(Side::BID, 1000) == 50);
    assert(book.get_volume_at_price(Side::ASK, 1000) == 0);
    std::cout << "[PASS] test_add_order\n";
}

void test_modify_order() {
    OrderBook book;
    OrderMessage msg;
    msg.price_ticks = 1000;
    msg.quantity = 50;
    msg.side = Side::BID;
    msg.action = OrderAction::ADD;
    book.apply(msg);

    msg.quantity = 20;
    msg.action = OrderAction::MODIFY;
    book.apply(msg);

    assert(book.get_volume_at_price(Side::BID, 1000) == 20);
    std::cout << "[PASS] test_modify_order\n";
}

void test_cancel_order() {
    OrderBook book;
    OrderMessage msg;
    msg.price_ticks = 1000;
    msg.quantity = 50;
    msg.side = Side::BID;
    msg.action = OrderAction::ADD;
    book.apply(msg);

    msg.quantity = 50;
    msg.action = OrderAction::CANCEL;
    book.apply(msg);

    assert(book.get_volume_at_price(Side::BID, 1000) == 0);
    std::cout << "[PASS] test_cancel_order\n";
}

void test_cancel_underflow() {
    OrderBook book;
    OrderMessage msg;
    msg.price_ticks = 1000;
    msg.quantity = 10;
    msg.side = Side::BID;
    msg.action = OrderAction::ADD;
    book.apply(msg);

    msg.quantity = 50; // Try to cancel more than exists
    msg.action = OrderAction::CANCEL;
    book.apply(msg);

    assert(book.get_volume_at_price(Side::BID, 1000) == 0); // Should clamp to 0
    std::cout << "[PASS] test_cancel_underflow\n";
}

void test_out_of_bounds() {
    OrderBook book;
    OrderMessage msg;
    msg.price_ticks = 9999; // Above LOB_ARRAY_SIZE
    msg.quantity = 10;
    msg.side = Side::BID;
    msg.action = OrderAction::ADD;
    
    // Should safely ignore and not segfault
    book.apply(msg);
    std::cout << "[PASS] test_out_of_bounds\n";
}

int main() {
    std::cout << "Running OrderBook Tests...\n";
    test_add_order();
    test_modify_order();
    test_cancel_order();
    test_cancel_underflow();
    test_out_of_bounds();
    std::cout << "All OrderBook Tests Passed!\n";
    return 0;
}
