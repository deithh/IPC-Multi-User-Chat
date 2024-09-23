# IPC chat
This project implements a simple chat server and client system using IPC (Inter-Process Communication) message queues in C. The client application allows users to send and receive messages, create and subscribe to topics, and block or unblock other users. The server handles user management, message distribution, and topic management. Both client and server processes communicate through IPC message queues, ensuring synchronous message passing and efficient inter-process communication.


## Setup and Commands:

start the server: 
./server

exit: ctrl + c 

start the client:
./client [nick 0-31chars]

Available Commands:

/help - Display help information
/createThread - Create a new topic
/listThreads - Request the list of topics from the server
/subThread - Subscribe to a thread
/requestPosts - Request synchronous messages
/post - Create and send a message
/blockUser - Block a user
/unblockUser - Unblock a user
/exit - Exit the client

These commands guide users to retrieve necessary data and send requests.

## Files Description
client.c - This file contains the implementation of the chat client program using IPC message queues in C. After logging in, the client can execute various commands, such as sending and receiving messages, subscribing to topics, creating new threads, and blocking other users. The code also handles server messages and manages client-server interactions through child processes.

server.c - This file contains the implementation of the chat server, which facilitates communication between clients via IPC message queues. The server supports user login, topic creation, thread subscription, and the sending and receiving of messages between users. Each type of client request is handled by a corresponding function, and messages are processed using child processes.

models.h - This header file defines global variables using macros and introduces the necessary structures used in the client and server programs.
