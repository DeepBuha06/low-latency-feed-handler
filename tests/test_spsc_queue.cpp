#include <iostream>
#include <cstdlib>
#include "../third_party/spsc_queue/include/spsc_queue.h"

#define ASSERT_TRUE(cond) do { if (!(cond)) { std::cerr << "Assert Failed: " << #cond << " at line " << __LINE__ << "\n"; std::exit(1); } } while(0)

struct TestItem {
    int id;
};

void test_push_pop() {
    SPSCQueue<TestItem, 4> queue;
    ASSERT_TRUE(queue.empty());
    ASSERT_TRUE(queue.size() == 0);

    TestItem item{10};
    bool success = queue.push(item);
    ASSERT_TRUE(success);
    ASSERT_TRUE(!queue.empty());
    ASSERT_TRUE(queue.size() == 1);

    TestItem out;
    success = queue.pop(out);
    ASSERT_TRUE(success);
    ASSERT_TRUE(out.id == 10);
    ASSERT_TRUE(queue.empty());

    std::cout << "[PASS] test_push_pop\n";
}

void test_full() {
    SPSCQueue<TestItem, 4> queue;
    ASSERT_TRUE(queue.push({1}));
    ASSERT_TRUE(queue.push({2}));
    ASSERT_TRUE(queue.push({3}));
    ASSERT_TRUE(queue.push({4}));
    
    // 5th push should fail because capacity is 4
    ASSERT_TRUE(!queue.push({5}));
    ASSERT_TRUE(queue.size() == 4);

    std::cout << "[PASS] test_full\n";
}

void test_empty() {
    SPSCQueue<TestItem, 4> queue;
    TestItem out;
    ASSERT_TRUE(!queue.pop(out)); // Should fail gracefully
    std::cout << "[PASS] test_empty\n";
}

void test_wraparound() {
    SPSCQueue<TestItem, 4> queue;
    
    // Push 4, pop 4 (head and tail move to 4)
    for(int i=0; i<4; ++i) ASSERT_TRUE(queue.push({i}));
    for(int i=0; i<4; ++i) {
        TestItem out;
        ASSERT_TRUE(queue.pop(out));
        ASSERT_TRUE(out.id == i);
    }

    // Now push another 2. The internal buffer index should wrap to 0 and 1.
    ASSERT_TRUE(queue.push({100}));
    ASSERT_TRUE(queue.push({101}));

    TestItem out;
    ASSERT_TRUE(queue.pop(out));
    ASSERT_TRUE(out.id == 100);
    ASSERT_TRUE(queue.pop(out));
    ASSERT_TRUE(out.id == 101);

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
