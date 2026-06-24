#!/bin/bash
cd /mnt/c/low-latency-feed-handler

echo "Starting Feed Handler in the background..."
./build/feed_handler &
HANDLER_PID=$!

echo "Waiting 1 second for feed handler to initialize..."
sleep 1

echo "Starting Market Simulator..."
python3 scripts/market_simulator.py

echo "Sending SIGINT to Feed Handler (PID $HANDLER_PID) for graceful shutdown..."
kill -SIGINT $HANDLER_PID

# Wait for the feed handler to cleanly flush telemetry to CSV
wait $HANDLER_PID

echo "Generating Latency Histogram..."
python3 scripts/plot_histogram.py
