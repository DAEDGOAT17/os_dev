#!/bin/bash
set -e
./run.sh &
PID=$!
sleep 5
kill $PID
cat serial.log || true
