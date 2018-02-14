#!/bin/bash

./server 5954 > /dev/null &
SERVER_PID=$!
echo "Server PID is $SERVER_PID."
./garbage localhost 5954 commondefs.h
echo "Killing $SERVER_PID."
kill $SERVER_PID
