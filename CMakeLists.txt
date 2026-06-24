cmake_minimum_required(VERSION 3.16)
project(FeedHandler CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# HFT Toolchain (Strict Warnings, Max Optimization, Link-Time Optimization)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -flto -Wall -Wextra -Werror -Wno-interference-size")

# Note on OS compatibility:
# This project enforces Linux kernel headers (SO_TIMESTAMPING). It will compile on WSL2,
# but hardware timestamping is only truly active on native Linux hardware.

# Include directories
include_directories(include)
include_directories(third_party/spsc_queue/include)

# Executable
add_executable(feed_handler 
    src/main.cpp 
    src/feed_handler.cpp 
    src/order_book.cpp
)

find_package(Threads REQUIRED)
target_link_libraries(feed_handler Threads::Threads)
