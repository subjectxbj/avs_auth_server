#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<sys/wait.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<signal.h>
#include<fcntl.h>
#include "cJSON.h"
#include "lwa.h"
#include "config.h"

#define CONNMAX 1000
#define BYTES 1024
#define PORT "3000"
#define CONFIG_IN "AlexaClientSDKConfig.json"
#define CONFIG_OUT "AlexaClientSDKConfig_out.json"
#define REDIRECT_URI "http://localhost:3000/authresponse"

char config_in_path[256]={0};
char config_out_path[256]={0};
char redirect_uri[256]={0};
char port[16]={0};
int listenfd, clients[CONNMAX];
cJSON *json_config = NULL;


void error(char *);
void startServer(char *);
void respond(int);


#ifdef HARDCODE_CONFIG
#pragma message("Use hard coded config")  
#endif

int main(int argc, char** argv)
{
	struct sockaddr_in clientaddr;
	socklen_t addrlen;
	int c;    

	int slot=0;

	strcpy(config_in_path, CONFIG_IN);
	strcpy(config_out_path, CONFIG_OUT);
	strcpy(port, PORT);
	strcpy(redirect_uri, REDIRECT_URI);
	//Parsing the command line arguments

	while ((c = getopt (argc, argv, "p:i:o:r:")) != -1) {
		switch (c)
		{
			case 'i':
				strcpy(config_in_path, optarg);
				break;
			case 'o':
				strcpy(config_out_path, optarg);
				break;
			case 'p':
				strcpy(port,optarg);
				break;
			case 'r':
				strcpy(redirect_uri,optarg);
				break;
			default:
				printf("\nERROR: Unknown parameter\n");
				exit(-1);
		}
	}

	printf("AVS Auth Server started.\nPort: %s%s%s \nConfig in: %s%s%s \nConfig out: %s%s%s\nRedirect URI: %s%s%s\n",
	"\033[92m",port,"\033[0m",
	"\033[92m",config_in_path,"\033[0m",
	"\033[92m",config_out_path,"\033[0m",
	"\033[92m",redirect_uri,"\033[0m");
	
	json_config = getCleanConfig(config_in_path);
	if (json_config == NULL) {
		printf("ERROR: failed to parse the input config");
		exit(-1);
	}

	// Setting all elements to -1: signifies there is no client connected
	int i;
	for (i=0; i<CONNMAX; i++)
		clients[i]=-1;
	startServer(port);

	// ACCEPT connections
	while (1)
	{
		addrlen = sizeof(clientaddr);
		clients[slot] = accept (listenfd, (struct sockaddr *) &clientaddr, &addrlen);

		if (clients[slot]<0)
			error ("accept() error");
		else
		{
			if ( fork()==0 )
			{
				respond(slot);
				exit(0);
			}else{
				int status;
				if (waitpid(-1, &status, 0) >0 )
				{
					if (WEXITSTATUS(status) == 0){
						printf("\nSUCCESS\n");
						exit(0);
					}
				}
			}
		}

		while (clients[slot]!=-1) slot = (slot+1)%CONNMAX;
	}

	return 0;
}

//start server
void startServer(char *port)
{
	struct addrinfo hints, *res, *p;

	// getaddrinfo for host
	memset (&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo( NULL, port, &hints, &res) != 0)
	{
		perror ("getaddrinfo() error");
		exit(1);
	}
	// socket and bind
	for (p = res; p!=NULL; p=p->ai_next)
	{
		listenfd = socket (p->ai_family, p->ai_socktype, 0);
		if (listenfd == -1) continue;
		if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) break;
	}
	if (p==NULL)
	{
		perror ("socket() or bind()");
		exit(1);
	}

	freeaddrinfo(res);
	unsigned char bOptVal= 1;
	int bOptLen = sizeof(bOptVal);
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (void*)&bOptVal, bOptLen);

	// listen for incoming connections
	if ( listen (listenfd, 1000000) != 0 )
	{
		perror("listen() error");
		exit(1);
	}
}

//client connection
void respond(int n)
{
	char mesg[99999], *reqline[3], data_to_send[BYTES], path[99999];
	int rcvd, fd, bytes_read;
	int ret=0;

	memset( (void*)mesg, (int)'\0', 99999 );

	rcvd=recv(clients[n], mesg, 99999, 0);
    printf("\nINFO: Received request from client %d\n", n);

	if (rcvd<0)    // receive error
		fprintf(stderr,("recv() error\n"));
	else if (rcvd==0)    // receive socket closed
		fprintf(stderr,"Client disconnected upexpectedly.\n");
	else    // message received
	{
		printf("%s", mesg);
		reqline[0] = strtok (mesg, " \t\n");
		if ( strncmp(reqline[0], "GET\0", 4)==0 )
		{
			reqline[1] = strtok (NULL, " \t");
			reqline[2] = strtok (NULL, " \t\n");
			if ( strncmp( reqline[2], "HTTP/1.0", 8)!=0 && strncmp( reqline[2], "HTTP/1.1", 8)!=0 )
			{
				write(clients[n], "HTTP/1.0 400 Bad Request\n", 25);
			}
			else
			{
				
				if ( strncmp(reqline[1], "/\0", 2)==0 ) {
					ret = handleUserRequest(clients[n]);
				}else if (strncmp(reqline[1], "/authresponse", 13)==0) {
					ret = handleAuthCodeGrant(clients[n], reqline[1]);
					if (ret == 0) {
						printf("\nGOOD: successfully generated the config with refresh token. Exit.\n");
						exit(0);
					}
				}
			}
		}
	}

	//Closing SOCKET
	shutdown (clients[n], SHUT_RDWR);         //All further send and recieve operations are DISABLED...
	close(clients[n]);
	clients[n]=-1;
	exit(1);
}
