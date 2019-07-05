# TCP-Server-Client-Chat-Application
Implementing a multi client, single server chat service on TCP

#Compilation:# gcc -pthread program.c -o program

#Execution#: ./server port_no max_connections_allowed
             ./client IP_address port_no
             
#Commands for Server:#
  1. 0 message <Enter, Ctrl + D> : Broadcast a message to all the clients
  2. i message <Enter, Ctrl + D> : Server sends message to Client labelled as 'i'
  3. 0 q <Enter, Ctrl + D> : Server and all clients terminate session
  4. i q <Enter, Ctrl + D> : Client numbered 'i' terminates connection to server
  
#Commands for Client:#
  1. 0 message <Enter, Ctrl + D> : Client sends message to server
  2. i message <Enter, Ctrl + D> : Client sends message to Client numbered 'i'
  3. q <Enter, Ctrl + D> : Client terminates connection to server
