#!/bin/sh

# Simple stress test script for SimpleHTTP server
CONNECTIONS="5 10 20 50 100 200 500 1000 2000 5000"

FILENAME="stress_test_results_`date +%Y%m%d_%H%M%S`.txt"

PORT=8180
for conn in $CONNECTIONS; do
    # Start the server
    echo "Testing with $conn connections..."
    ./build/ssfhs -d examples/stress_test -c examples/stress_test/ssfhs.conf -p $PORT &
    SPID=$!
    sleep 1

    # Stress test with wrk
    echo "Connections: $conn" >> $FILENAME
    wrk -t4 -c$conn -d10s http://localhost:$PORT/ >> $FILENAME 2>&1

    # Stop the server
    sleep 5
    kill $SPID
    wait $SPID 2>/dev/null

    # Increment port to avoid conflicts
    PORT=$((PORT + 1))
done