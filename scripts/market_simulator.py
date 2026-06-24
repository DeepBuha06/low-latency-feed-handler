import socket
import struct
import time
import random

UDP_IP = "127.0.0.1"
UDP_PORT = 12345

# struct OrderMessage {
#     uint32_t sequence_number; 
#     uint64_t timestamp_ns;    
#     uint64_t order_id;
#     uint32_t symbol_id;
#     uint8_t action;
#     uint8_t side;
#     uint32_t price_ticks;     
#     uint32_t quantity;
# };
# Pack format: < I Q Q I B B I I (Little Endian)
# I (4) + Q (8) + Q (8) + I (4) + B (1) + B (1) + I (4) + I (4) = 34 bytes

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
print(f"Blasting UDP packets to {UDP_IP}:{UDP_PORT}...")

seq_num = 1
start_time = time.time()

for i in range(1000000):
    # Simulate ~1% packet loss (as an HFT requirement test)
    if random.random() < 0.01:
        seq_num += 1
        continue
        
    timestamp_ns = int(time.time_ns())
    order_id = i
    symbol_id = 1
    action = 1 # ADD
    side = 1 # BID
    price_ticks = random.randint(0, 1999) # Must fit in 0 to LOB_ARRAY_SIZE-1
    qty = random.randint(1, 100)
    
    msg = struct.pack("<IQQIBBII", seq_num, timestamp_ns, order_id, symbol_id, action, side, price_ticks, qty)
    sock.sendto(msg, (UDP_IP, UDP_PORT))
    
    seq_num += 1
    
    # Throttle slightly to prevent loopback buffer overflow
    if i % 10000 == 0:
        time.sleep(0.001)

print(f"Finished sending 1M packets in {time.time() - start_time:.2f} seconds.")
