./server 5954 > /dev/null 2> /dev/null &
SERVER_PID=$!
./client localhost 5954 commondefs.h
kill $SERVER_PID
