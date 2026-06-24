#include <iostream>
#include <cassert>
#include "../third_party/spsc_queue/include/spsc_queue.h"

struct TestItem {
    int id;
};

void test_push_pop() {
    SPSCQueue<TestItem, 4> queue;
    assert(queue.empty());
    assert(queue.size() == 0);

    TestItem item{10};
    bool success = queue.push(item);
    assert(success);
    assert(!queue.empty());
    assert(queue.size() == 1);

    TestItem out;
    success = queue.pop(out);
    assert(success);
    assert(out.id == 10);
    assert(queue.empty());

    std::cout << "[PASS] test_push_pop\n";
}

void test_full() {
    SPSCQueue<TestItem, 4> queue;
    assert(queue.push({1}));
    assert(queue.push({2}));
    assert(queue.push({3}));
    assert(queue.push({4}));
    
    // 5th push should fail because capacity is 4
    assert(!queue.push({5}));
    assert(queue.size() == 4);

    std::cout << "[PASS] test_full\n";
}

void test_empty() {
    SPSCQueue<TestItem, 4> queue;
    TestItem out;
    assert(!queue.pop(out)); // Should fail gracefully
    std::cout << "[PASS] test_empty\n";
}

void test_wraparound() {
    SPSCQueue<TestItem, 4> queue;
    
    // Push 4, pop 4 (head and tail move to 4)
    for(int i=0; i<4; ++i) assert(queue.push({i}));
    for(int i=0; i<4; ++i) {
        TestItem out;
        assert(queue.pop(out));
        assert(out.id == i);
    }

    // Now push another 2. The internal buffer index should wrap to 0 and 1.
    assert(queue.push({100}));
    assert(queue.push({101}));

    TestItem out;
    assert(queue.pop(out));
    assert(out.id == 100);
    assert(queue.pop(out));
    assert(out.id == 101);

    std::cout << "[PASS] test_wraparound\n";
}

int main() {
    std::cout << "Running SPSCQueue Tests...\n";
    test_push_pop();
    test_full();
    test_empty();
    test_wraparound();
    std::cout << "All SPSCQueue Tests Passed!\n";
    return 0;
}
