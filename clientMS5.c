/*
* @author Omer Farooq Ahmed 
* version 6.0.0/milestone4.2
* this program is the client
* for the shell 
*/

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>

int glob=0;

void *read_thread(void *sock) {
	while(1) {
		//read and display output from server
		char output[1024];
		int *socket=(int *)sock;
		int val=*socket;
		int servc=read(val, output, 1024);
		if (servc==-1) {perror("socket read"); exit(1);}
		output[servc]='\0';
		write(STDOUT_FILENO, "\n", 1);
		write(STDOUT_FILENO, output, servc); 
		write(STDOUT_FILENO, "\n", 1);
		
		if (strcmp(output, "disconnected")==0) {
			glob=1;
		}
	}
}
int main() { 
	while (1) {
		//take connection arguments
		const char message[]="Please enter a command:\n1. connect [IP Address] [Port]\n2. exit\n";
		write(STDOUT_FILENO, message, sizeof(message));
		char *list[3];
		char inbuff[100];
		int incount=read(STDIN_FILENO, inbuff, 100);
		inbuff[incount-1]='\0';
		//tokenize input string 
		const char delim[]=" ";
		char *token;
		token=strtok(inbuff, delim);
		const char newline[]="\n";
		int i=0;
		while (token!= NULL) {
			list[i]=token;
			i++;
			token=strtok(NULL,delim);
		}
		if (strcmp(list[0], "exit")==0) {
			//debugging 
			write(STDOUT_FILENO, "exiting\n", 8);
			break;
		}
		
		//setup connection
		int sock, conn_status;
		struct sockaddr_in server;
		//create a socket
		sock=socket(AF_INET, SOCK_STREAM, 0);
		if (sock==-1) {perror("socket"); exit(1);}
		//set server information
		server.sin_family=AF_INET;
		server.sin_port=htons(atoi(list[2]));
		server.sin_addr.s_addr=inet_addr(list[1]);
		//connect to server
		conn_status=connect(sock, (struct sockaddr *)&server, sizeof(server));
		if (conn_status==-1) {perror("connection"); exit(1);}
	
		//Display connected
		char conn_msg[]="Connected to ";
		strcat(conn_msg, list[1]);
		strcat(conn_msg, ":");
		strcat(conn_msg, list[2]);
		write(STDOUT_FILENO, conn_msg, 14+strlen(list[1])+strlen(list[2]));
		write(STDOUT_FILENO, newline, strlen(newline));
		write(STDOUT_FILENO, newline, strlen(newline));
		
		//create thread for reading from socket and writing on the screem
		pthread_t r_thread;
		int ret=pthread_create(&r_thread, NULL, read_thread, (void *)&sock);
		if (ret) {perror("thread"); exit(1);}
		
		while (1) {
			//take input arguments
			const char buff1[]="Please enter your command\n";
			write (STDOUT_FILENO, buff1, strlen(buff1));
			char buff2[100];
			int count=read (STDIN_FILENO, buff2, 100);
			buff2[count-1]='\0';
			
			//send input to server
			if (send(sock, buff2, count, 0)==-1) {perror("send"); exit(1);}
			if (strcmp(buff2, "disconnect")==0) {
				break;
			}
			
		}
	}
	//close(sock);
	return 0;
}
