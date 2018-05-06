/*
* @author Omer Farooq Ahmed 
* version 7.0.1/milestone5.0
* this program is the shell server
* that serves multiple clients
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>              
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>


//structure for client list
typedef struct client {
	int pid;
	char ip[50];
	int cli_port;
	int write_pipe[2];
	int read_pipe[2];
	int sockfd;
} aclient;

//setup clients structure array
aclient clients[5];
int client_count=0;

//NODE for process list
typedef struct process {
	char name[30];
	pid_t pid;
	char status[30];
	char start_time[9];
	char end_time[9];
	struct process *next;
} node;

//Structure for time
struct time {
	int hour, minute, second;
};
void calcTimeDiff(struct time t1, struct time t2, struct time *t3);

//interactivity thread function 
void * interactive_thread(void *arg) {
	while (1) {
		//take command input from user
		const char interactive_msg[]="Welcome to Omer's server. Please enter a server command:\n1. List all clients (clients)\n2. list client by ip address (client [ip-address])\n3. Show all process lists (lists)\n";
		write(STDOUT_FILENO, interactive_msg, strlen(interactive_msg));
		char input[100];
		int count=read(STDIN_FILENO, input, 100);
		input[count-1]='\0';
		char *list[5];
		// Begin tokenization and entering tokens in list
		const char delim[]=" ";
		char *token;
		token=strtok(input, delim);
		const char newline[]="\n";
		int i=0;
		while (token!= NULL) {
			list[i]=token;
			i++;
			token=strtok(NULL,delim);
		}
		//check to see if list all clients
		
		//debugging
		write(STDOUT_FILENO, "\n", 1);
		write(STDOUT_FILENO, list[0], strlen(list[0]));
		write(STDOUT_FILENO, "\n", 1);
		
		//list all client info
		if (strcmp(list[0], "clients")==0) {
			
			for (int i=0; i<client_count; i++) {
				write(STDOUT_FILENO, "client:\nPID: ", 11);
				char idout[10];
				int idcount=sprintf(idout, "%d", clients[i].pid);
				write(STDOUT_FILENO, idout, idcount);
				write(STDOUT_FILENO, "\nIP ADDRESS/PORT: ", 18);
				char ipout[20];
				strcpy(ipout, clients[i].ip);
				write(STDOUT_FILENO, ipout, strlen(ipout));
				write(STDOUT_FILENO, ":", 1);
				char portout[10];
				int portcount=sprintf(portout, "%d", clients[i].cli_port);
				write(STDOUT_FILENO, portout, portcount);
				write(STDOUT_FILENO, "\n\n", 2);
				
			}
		}
		//list only specific client info
		if (strcmp(list[0], "client")==0  && i>1) {
			for (int j=0; j<client_count; j++) {
				if (clients[j].pid==atoi(list[1])) {
					write(STDOUT_FILENO, "client:\nPID: ", 11);
					char idout[10];
					int idcount=sprintf(idout, "%d", clients[j].pid);
					write(STDOUT_FILENO, idout, idcount);
					write(STDOUT_FILENO, "\nIP ADDRESS/PORT: ", 18);
					char ipout[20];
					strcpy(ipout, clients[j].ip);
					write(STDOUT_FILENO, ipout, strlen(ipout));
					write(STDOUT_FILENO, ":", 1);
					char portout[10];
					int portcount=sprintf(portout, "%d", clients[j].cli_port);
					write(STDOUT_FILENO, portout, portcount);
					write(STDOUT_FILENO, "\n\n", 2);
				}
			}
		}
		//show all process lists
		if (strcmp(list[0], "lists")==0) {
			//debugging
			write(STDOUT_FILENO, "entering lists\n", 15);
			
			char all_lists[5000];
			strcat(all_lists, "All Lists:\n\n");
		
			for (int j=0; j<client_count; j++) {
				//close read end of write pipe
				close(clients[j].write_pipe[0]);
				//write to the pipe
				if (write(clients[j].write_pipe[1], "list", 4)==-1) {perror("writepipe write");}
				//debugging 
				write(STDOUT_FILENO, "not blocked yet\n", 16);
				
				//sleep(0.3);
				//close write end of read pipe
				close(clients[j].read_pipe[1]);
				//read process list
				char listbuff[1024];
				int list_count=read(clients[j].read_pipe[0], listbuff, 1024);
				//debugging 
				write(STDOUT_FILENO, "still not blocked\n", 18);
				//write(STDOUT_FILENO, listbuff, list_count);
				
				if (list_count==-1) {perror("readpipe read");}
				listbuff[list_count]='\0';
				strcat(all_lists, listbuff);
				strcat(all_lists, "\n");
			}
			if(write(STDOUT_FILENO, all_lists, strlen(all_lists))==-1) {perror("all lists write");}
		
		}
		
	}
	
}
node *glob_head;



//Print all the NODEs in the pocess list
char * print_list(node *head) {
	const char newline[]="\n";
	const char tab[]="\t";
	char *buff=malloc(2000);
	strcpy(buff, "\nNAME\t\tPID\t\tSTATUS\t\tSTART TIME\t\tEND TIME\t\tTIME ELAPSED\n");
	node *current=head;
	while (current!=NULL) {
		strcat(buff, current->name);
		strcat(buff, "\t");
		strcat(buff, "\t");
		char id[20];
		int count=sprintf(id, "%d", current->pid);
		strcat(buff, id);
		strcat(buff, "\t");
		strcat(buff, "\t");
		strcat(buff, current->status);
		strcat(buff, "\t");
		strcat(buff, "\t");
		strcat(buff, current->start_time);
		strcat(buff, "\t");
		strcat(buff, "\t");
		strcat(buff, current->end_time);
		//elapsed time
		if (strcmp(current->end_time, " ")!=0) {
			struct time t1,t2,t3;
			//populate t1 with start time values
			char t1_hr[2];
			memcpy(t1_hr, &current->start_time[0], 2);
			t1_hr[2]='\0';
			t1.hour=atoi(t1_hr);
			char t1_m[2];
			memcpy(t1_m, &current->start_time[3], 2);
			t1_m[2]='\0';
			t1.minute=atoi(t1_m);
			char t1_s[2];
			memcpy(t1_s, &current->start_time[6], 2);
			t1_s[2]='\0';
			t1.second=atoi(t1_s);
			//populate t2 with end time values
			char t2_hr[2];
			memcpy(t2_hr, &current->end_time[0], 2);
			t2_hr[2]='\0';
			t2.hour=atoi(t2_hr);
			char t2_m[2];
			memcpy(t2_m, &current->end_time[3], 2);
			t2_m[2]='\0';
			t2.minute=atoi(t2_m);
			char t2_s[2];
			memcpy(t2_s, &current->end_time[6], 2);
			t2_s[2]='\0';
			t2.second=atoi(t2_s);
			//call difference function
			calcTimeDiff(t2, t1, &t3);
			strcat(buff, "\t");
			strcat(buff, "\t");
			char time_diff[50];
			int time_count=sprintf(time_diff, "%02d:%02d:%02d", t3.hour, t3.minute, t3.second);
			strcat(buff, time_diff);
		}
		strcat(buff, "\n");
		current=current->next;
	}
	return buff;
}

//list communication thread TO-DO
void *pipe_thread(void *arg) {
	while(1) {
		int count=client_count;
		//read from read pipe
		close(clients[count-1].write_pipe[1]);
		char input[4];
		int pipecount=read(clients[count-1].write_pipe[0], input, 4);
		//input[pipecount]='\0';
		if (strcmp(input, "list")==0) {
			//write process list
			close(clients[count-1].read_pipe[0]);
			char *output=print_list((node *)arg);
			//debugging 
			//write(STDOUT_FILENO, output, strlen(output));
			
			if(write(clients[count-1].read_pipe[1], output, strlen(output))==-1) {perror("list write pipe");}
		}
	}

}

//Print all the Active NODEs
char * print_active(node *head) {
	const char newline[]="\n";
	const char tab[]="\t";
	char *buff=malloc(2000);
	strcpy(buff, "\nNAME\t\tPID\t\tSTATUS\t\tSTART TIME\t\tEND TIME\n");
	node *current=head;
	while (current!=NULL) {
		if (strcmp(current->status, "Active")==0) {
			strcat(buff, current->name);
			strcat(buff, "\t");
			strcat(buff, "\t");
			char id[20];
			int count=sprintf(id, "%d", current->pid);
			strcat(buff, id);
			strcat(buff, "\t");
			strcat(buff, "\t");
			strcat(buff, current->status);
			strcat(buff, "\t");
			strcat(buff, "\t");
			strcat(buff, current->start_time);
			strcat(buff, "\t");
			strcat(buff, "\t");
			strcat(buff, current->end_time);
			strcat(buff, "\n");
		}
		current=current->next;
	}
	return buff;
}

//Add a NODE to the end of the list
void push_list(node *head, char *aname, pid_t apid, char *astatus, char *astart_time, char *aend_time) {
	node *current=head;
	while (current->next!=NULL) {
		current=current->next;
	}	
	current->next=malloc(sizeof(node));
	strcpy(current->next->name, aname);
	//current->next->name=aname;
	//current->next->name[strlen(aname)-1]='\0';
	current->next->pid=apid;
	strcpy(current->next->status, astatus);
	strcpy(current->next->start_time, astart_time);
	strcpy(current->next->end_time, aend_time);
	current->next->next=NULL;
}

//Remove a NODE from the end of the list
void pop_node(node *head) {
	node *current=head;
	while (current->next->next!=NULL) {
		current=current->next;
	}
	current->next=NULL;
}

//Change Node status
void kill_node (node *head, int id, char *signal) {
	time_t curr_time;
	struct tm * time_info;
	char endtime[9];
	time(&curr_time);
	time_info=localtime(&curr_time);
	strftime(endtime, sizeof(endtime), "%H:%M:%S", time_info);
	node *current=head;
	while (current->pid!=id) {
		current=current->next;
	}
	strcpy(current->status, signal);
	if (strcmp(signal, "Dead")==0) {
		strcpy(current->end_time, endtime);
		
	}
}

int childid;
int status;
int valid=0;
//handling SIGCHLD
void handler1(int signum) {
	childid=waitpid(-1, &status, WNOHANG);
	//if (childid>0) {
	//	valid++;
	//}
	kill_node(glob_head, childid, "Dead");
}

void calcTimeDiff(struct time t1, struct time t2, struct time *t3) {
    //calculate difference
    //get time in total seconds
    int seconds1 = t1.hour*60*60 + t1.minute*60 + t1.second;
    int seconds2 = t2.hour*60*60 + t2.minute*60 + t2.second;
    int totalSeconds = seconds1-seconds2;
    //extract time in Hours, Minutes and Seconds
    t3->minute = totalSeconds/60;
    t3->hour = t3->minute/60;
    t3->minute = t3->minute%60;
    t3->second = totalSeconds%60;
}

int kill_by_name(node *head, char *aname) {
	node *current=head;
	while ((strcmp(aname, current->name)!=0 && current->next!=NULL) || (strcmp(aname, current->name)==0 && strcmp(current->status, "Dead")==0 && current->next!=NULL)) {
		current=current->next;
	}
	if (current->next==NULL && strcmp(current->name, aname)!=0) {
		return -1;
	}
	else {
		kill_node(head, current->pid, "Dead");
		//strcpy(current->status, "Dead");
		return current->pid;
	}
}

const char newline[]="\n";
int main(int argc, char *argv[]) {
	//Node assignment
	node *head=NULL;
	head= malloc(sizeof(node));
	const char parent[]="Shell";
	const char shellstatus[]="Active";
	strcpy(head->name, parent);
	head->name[strlen(parent)]='\0';
	head->pid= getpid();
	strcpy(head->status, shellstatus);
	//assign time information
	time_t current_time;
	struct tm * time_info;
	char timestring[9]; //space for "HH:MM:SS\0"
	time(&current_time);
	time_info=localtime(&current_time);
	strftime(timestring, sizeof(timestring), "%H:%M:%S", time_info);
	//puts(timestring);
	strcpy(head->start_time, timestring);
	strcpy(head->end_time, " ");
	head->next=NULL;
	glob_head=head;
	//set SIGCHLD signal 
	__sighandler_t sigval=signal(SIGCHLD, handler1);
	if (sigval==SIG_ERR){
		perror("signal");
	}
	
	//SETUP SERVER
	int serv_sock, client_sock;
	struct sockaddr_in server, clientaddr;
	char buff[1024];
	int incount;
	int clientaddr_len=sizeof(clientaddr);
	pid_t cpid;
	//create socket
	serv_sock=socket(AF_INET, SOCK_STREAM, 0);
	if (serv_sock==-1) {perror("socket"); exit(1);}
	//set socket information
	server.sin_family=AF_INET;
	server.sin_addr.s_addr=INADDR_ANY;
	server.sin_port=htons(atoi(argv[1]));
	//bind the socket to server's address
	if (bind(serv_sock, (struct sockaddr *)&server, sizeof(server))==-1) {
		perror("binding1");
		exit(1);
	}
	//listen for connections
	if (listen(serv_sock, 5)==-1) {perror("listen"); exit(1);}
	
	
	//create interactive thread
	pthread_t inter_thread;
	int thret1=pthread_create(&inter_thread, NULL, interactive_thread, NULL);
	if (thret1) {perror("interactive thread"); }
	
	while (1) {
		//accept connections
		if ((client_sock=accept(serv_sock, (struct sockaddr *)&clientaddr, &clientaddr_len))==-1) {
			perror("accept");
			exit(1);
		}
		//increase the number of clients
		client_count++;
		//create pipe with child to get its list. parent will read and child will write list before disconnectings
		int list_write_pipe[2];
		int list_read_pipe[2];
		if (pipe(list_write_pipe)==-1) {perror("write list pipe"); exit(1);}
		if (pipe(list_read_pipe)==-1) {perror("read list pipe"); exit(1);}
		clients[client_count-1].write_pipe[0]=list_write_pipe[0];
		clients[client_count-1].write_pipe[1]=list_write_pipe[1];
		clients[client_count-1].read_pipe[0]=list_read_pipe[0];
		clients[client_count-1].read_pipe[1]=list_read_pipe[1];
		
		//fork into new child
		cpid=fork();
		if (cpid==-1) {perror("fork"); exit(1);}
		//parent process: fill in relevant details of child client into clients array
		if (cpid>0) {
			clients[client_count-1].pid=cpid;
			char str[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &(clientaddr.sin_addr), str, INET_ADDRSTRLEN);
			int cport=(int) ntohs(clientaddr.sin_port);
			char ipaddress[strlen(str)];
			strcpy(ipaddress, str);
			strcpy(clients[client_count-1].ip, str);
			clients[client_count-1].sockfd=client_sock;
			clients[client_count-1].cli_port=cport;
			
			
			
		}
		//child process
		if (cpid==0) {
			//concurrent pipe thread
			pthread_t p_thread;
			int thret2=pthread_create(&p_thread, NULL, pipe_thread, (void *)head);
			if (thret2==-1) {perror("pipe thread");}
			
			while (1) {
				
				char *list[30]; //This creates an array of character pointers (strings)
				//read arguments from socket
				char inbuff[100];
				if (read(client_sock, inbuff, 100)==-1) {perror("socket read"); exit(1);}
				// Begin tokenization and entering tokens in list
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
				//check for disconnect
				const char pexit[]="disconnect";
				if (strcmp(list[0], pexit)==0) {
					send(client_sock, "disconnected", 12, 0);
					break;
				}
				// Begin Addition operation
				const char add[]="add";
				if (strcmp(list[0], add)==0) {
					int result=0;
					for (int j=1; j<i; j++) {
						result+=atoi(list[j]);
					}
					char sum[50];
					int sumcount=sprintf(sum, "%d", result);
					//sum[sumcount-1]='\0';
					if (send(client_sock, sum, sumcount+1, 0)==-1) {perror("socket send add"); exit(EXIT_FAILURE);}	
				}
				// Begin Subtraction operation
				const char sub[]="sub";
				if (strcmp(list[0], sub)==0) {
					int result=0;
					if (i!=1) 
						result+=atoi(list[1]);
					for (int j=2; j<i; j++) {
						result-=atoi(list[j]);
					}
					char sum[50];
					int sumcount=sprintf(sum, "%d", result);
					if (send(client_sock, sum, sumcount+1, 0)==-1) {perror("socket send sub"); exit(EXIT_FAILURE);}
				}
				// Begin Multiply operation	
				const char mul[]="mul";
				if (strcmp(list[0], mul)==0) {
					int result=1;
					for (int j=1; j<i; j++) {
						result=result*atoi(list[j]);
					}
					char product[50];
					int productcount=sprintf(product, "%d", result);
					if (send(client_sock, product, productcount+1, 0)==-1) {perror("socket send mul"); exit(EXIT_FAILURE);}
				}
				// Begin Division operation
				const char div[]="div";
				if (strcmp(list[0], div)==0) {
					int result;
					if (i>1)
						result=atoi(list[1]);
					for (int j=2; j<i; j++) {
						result=result/atoi(list[j]);
					}
					char product[50];
					int productcount=sprintf(product, "%d", result);
					if (send(client_sock, product, productcount+1, 0)==-1) {perror("socket send div"); exit(EXIT_FAILURE);}
				}
				// Begin Exec operation
			
				const char run[]="run";
				if (strcmp(list[0], run)==0) {
					if (i>1) {
						char *file=list[1];
						char *args[i];
						char astatus[]="Active";
						for (int j=0; j<i-1; j++) {
							args[j]=list[j+1];
						}
						args[i-1]=NULL;
						//start time information
						time_t curr_time;
						struct tm * time_det;
						char starttime[9];
						time(&curr_time);
						time_det=localtime(&curr_time);
						strftime(starttime, sizeof(starttime), "%H:%M:%S", time_det);
						//puts(starttime);
						//create pipe for parent child communication
						int pipe3[2];
						if (pipe(pipe3)==-1) {perror("pipe3: "); exit(EXIT_FAILURE);}
						fcntl(pipe3[1], F_SETFD, FD_CLOEXEC);
						fcntl(pipe3[0], F_SETFD, FD_CLOEXEC);
						//fork to create new application process
						int cpid= fork();
						if (cpid==-1) {perror("fork: "); exit(0);}
						//child process
						if (cpid==0) {
							//close reading end of pipe
							close(pipe3[0]);
							pid_t pid= getpid();
							int err=execvp(file, args);
							if (err==-1) {
								perror("exec: ");
								if (write(pipe3[1], "FAILED\n", 7)==-1) {perror("pipe3 write "); exit(EXIT_FAILURE);}
								send (client_sock, "failed written\n", 15, 0);

							}
						}
						//parent process
						if (cpid!=0) {
							//close writing end of pipe
							close(pipe3[1]);
							//sleep(0.4);
							char in[7];
							int chldcount=read(pipe3[0], in, 7);
							if (chldcount==0) { //successfuly read from a closed pipe
								//adding the new process to the process list
								char emp[]=" ";
								push_list(head, file, cpid, astatus, starttime, emp);
								//debugging
								write(STDOUT_FILENO, "pushed\n", 7);
								if (send(client_sock, "execution complete\0", 19, 0)==-1) {perror("send run"); exit(EXIT_FAILURE);}
							}
						}
				
					}
					//if (send(client_sock, "no arguments for run\0", 21, 0)==-1) {perror("send run"); exit(EXIT_FAILURE);}
				}
				// List all operation
				const char palist[]="list-all";
				if (strcmp(list[0], palist)==0) {
					//if (valid>0) {
					//	char term[]="Dead";
					//	kill_node(head, childid, term);
					//	valid=0;
					//}
			
					char *output=print_list(head);
					if (send(client_sock, output, strlen(output)+1, 0)==-1) {perror("sock list all"); exit(EXIT_FAILURE);}
				}
				// List operation
				const char plist[]="list";
				if (strcmp(list[0], plist)==0) {
					//if (valid>0) {
					//	char term[]="Dead";
					//	kill_node(head, childid, term);
					//	valid=0;
					//}
				
					//print_active(head);
					char *output=print_active(head);
					char *output2=print_active(head);
					//debugging
					write(STDOUT_FILENO, output2, strlen(output2));
					write(STDOUT_FILENO, "\n", 1);
					
					if (send(client_sock, output2, strlen(output2), 0)==-1) {perror("sock list"); exit(EXIT_FAILURE);}
				}
				// Help operation
		
				const char help[]="help";
				if (strcmp(list[0], help)==0) {
					char helpbuff[]="\nWelcome to Omer's shell version 1.0\nchose from the following commands:	\n1. Add (add 1 2 ...)\n2. Subtract (sub 10 9 ...)\n3. Multiply (mul 1 2 3 ...)\n4. Divide (div 100 10 ...)\n5. Run (run gedit example.txt ...)\n6. List Running Processes (list)\n7. List All Processes (list-all)\n8. Kill (kill [process id] [signal]/ kill [process id / Name of process])\n9. Disconnect (disconnect)\n";
					if (send(client_sock, helpbuff, strlen(helpbuff)+1, 0)==-1) {perror("sock help"); exit(EXIT_FAILURE);}
				}
				// Kill operation
				const char pkill[]="kill";
				if (strcmp(list[0], pkill)==0) {
					if (i==3 && list[1]>0) {
						int errno= kill (atoi(list[1]), atoi(list[2]));
						if (errno==-1) {perror ("kill:");}
						kill_node(head, atoi(list[1]), list[2]);
					} 
					if (i==2 && list[1]>0) {
						//check if name is provided rather than process id
						if (list[1][0]!='0' && list[1][0]!='1' && list[1][0]!='2' && list[1][0]!='3' && list[1][0]!='4' && list[1][0]!='5' && list[1][0]!='6' && list[1][0]!='7' && list[1][0]!='8' && list[1][0]!='9') {
		
							//copy name into variable
							char *name;
							name=malloc(strlen(list[1]));
							strcpy(name, list[1]);
							int procid=kill_by_name(head, name);
							if (procid!=-1) {
								if (kill(procid, SIGTERM)==-1) {perror("kill");}
							}
						}
						else {
							int errno= kill (atoi(list[1]), SIGTERM);
							if (errno==-1) {perror ("kill:");}
							char term[]="Dead";
							kill_node(head, atoi(list[1]), term);
						}
					}
					if (send(client_sock, "kill successful\0", 16, 0)==-1) {perror("sock kill"); exit(EXIT_FAILURE);}
				}
				if (strcmp(list[0], pkill)!=0 && strcmp(list[0], help)!=0 && strcmp(list[0], plist)!=0 && strcmp(list[0], palist)!=0 && strcmp(list[0], run)!=0 && strcmp(list[0], div)!=0 && strcmp(list[0], mul)!=0 && strcmp(list[0], sub)!=0 && strcmp(list[0], add)!=0) { 
					if (send(client_sock, "illegal command\0", 16, 0)==-1) {perror("sock illegal"); exit(EXIT_FAILURE);}
				}
			}
			//exit from child
			exit(0);
		}
		
	}
}
