# UDP Remote Copy Client and Server

# Michael Bottini

## Building:

`make`

## Running:

`server <port>`
`client <hostname> <port> <filepath> <localpath>`

For example,

`./server 5954`
`./client localhost 5954 commondefs.h new.txt`

---

## Extra Functionality:

Note the ability to resolve hostnames; you can put in an IP address if you want
as well.

If no path is provided, the client will send the output to stdout. This allows
a user to pipe the output to a file. Info and errors go to stderr.

Check out the unit tests (`make stest`) for extra fun.
