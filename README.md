ClientServerShellMultiThread
System Programming 1839
Term Project Report

Omer Farooq Ahmed-09437

    1. Overview and Instructions
The term project is a multi-threaded/multiplexed IO client-server shell that performs basic mathematical operations and runs applications while also keeping a list of all processes. I have implemented the project with the following features and they are used as follows:
There are two files, namely, serverMS5.c and clientMS5.c. The client file has code dealing only with the client side of the project i.e. connecting to a particular server and then writing commands and reading the server’s reply displayed on the terminal. The server code reads the user’s inputs from a socket and processes the input and returns the result. The server also has its own interactive interface, where the user can get information about all the clients that the server has handled. 
In order to run the program, you have to first compile the server on a terminal and then run the executable by giving the port number as an argument. The file must also be compiled with the pthread flag because it uses POSIX threads. For example:
$ gcc serverMS5.c -pthread -o serv
$ ./serv 9001
The server program will execute and prompt the user to enter a command. The list of commands include “clients” which lists the details of all the clients that have connected to the server; “client [ip address]” which shows the details of the client with the specified IP address; “list” which displays the process lists of all the clients and “list [pid]” which lists down the process list of the client child process with the specified process ID. The prompt runs in an infinite loop, so the user can view the information whenever they like.
The client is also compiled with the pthread flag but does not take any parameters. As soon as the client program is executed, it prompts the user to enter one of two commands, namely, “connect [IP] [Port]” and “exit”. Exit terminates the program, while connect connects to a server based on the IP address and port number. The server must be running at the specified address and should be listening on the specified port. Once connected, the client program gives another prompt to enter a command. The following are legal commands:
    1. add [operand1] [operand2] …
    2. sub [operand1] [operand2] …
    3. mul [operand1] [operand2] …
    4. div [operand1] [operand2] …
    5. run [program name] [argument1] [argument2] …
    6. list-all (list all processes)
    7. list (list running processes)
    8. help (displays all commands list)
    9. kill [pid/name of process] 
    10. kill [pid] [signal]
    11. disconnect (disconnects from current server)
The list table shows the process name, process ID, process status (Active/Dead), start time, end time and time elapsed. After disconnecting, the client application shows the initial prompt again to connect to another server or exit (terminate).
    2. Architecture
The overall architecture of this project is a client server architecture with multithreading. I have chosen to use only multithreading rather than multiplexed I/O. The client application loops to write queries to a socket, that is bound to the server’s network address and port which is listening for TCP traffic. The server application listens for connections and loops to start accepting connections form multiple clients. As soon as a client establishes a connection with the server, the server’s main process forks and processes the queries within the child process, which has its own copy of the process list. The forking is necessary because each client needs its own copy of the structures and variables used. The client reads the user’s command and sends it on the socket and reads the reply from the server on the socket concurrently using multiple threads. The main thread loops and asks the user for commands, then sends the command on the socket. It then writes from a character buffer to the screen which is supposed to contain the reply. A pthread runs concurrently (created just before the main thread loop, with an infinite loop in its own function) and infinitely keeps reading the reply from the socket and copying it to the output buffer. 

The server also has multiple concurrent threads running. In this case there are three threads. The main thread forks whenever a connection is accepted and then in the child process, it reads the query from the socket, processes it and sends a reply on the socket. Another thread is responsible for prompting the user to enter a command and based on the command, to output information about the clients. This thread is created just before the main thread starts accepting connections in the main loop. This thread also runs an infinite loop. A third thread is responsible for reading from a pipe. This thread is created in the client child process and therefore there is an instance of it for every client child process. The pipe is used to communicate between the second thread and these client threads. When the second thread gets a command to get the process lists from each client, the thread runs a loop for each client and sends a request for process lists through one pipe and this is when the client thread that is read blocked, gets unblocked and then sends its process list through a second pipe to the interactive thread, which then displays all the process lists. 

Other features of the application architecture include:
    a. The process list is implemented as a linked list of a process structure. This is to ensure there is no limit to the number of processes that can be run with one client;
    b. An illegal application name provided with the run command is handled by a failed execvp, which causes the pipe used between the parent child exec handler processes to close due to fcntl setf cloexec. This returns a 0 when the parent process tries to read through the pipe, indicating a failed execution. Thus, an if statement then determines whether to push the illegal run process to the process list or not. 
    c. A signal handler for SIGCHLD is registered and the signal handler changes the status of the process which died since it has the terminated processes ID using wait with WNOHANG and a reference to the global process linked list head, through which it can traverse, looking for the matching process, in order to change its status to dead. 
    
    
    3. Limitations:
This project has the following limitations:
    a. The client program output strings are out of order due to multithreading;
    b. The client program only outputs the result of a mathematical operation, does not include “the result of add is”;
    c. The send and write functions on the socket do not consider the socket buffer size and do not initially send the length of the string to be transferred. Thus, if the process list sent to the client ever exceeds the buffer size for a socket, it will not be fully displayed at the client;
    d. The interactive thread in the server does not entertain message sending to the clients;
    e. No multiplexed I/O is used;
    f. The process list output is not in a consistent format if the process name is too long.
