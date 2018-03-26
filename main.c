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
#include "json.h"
#include "lwa.h"
#include "config.h"

#define CONNMAX 1000
#define BYTES 1024
#define PORT "3000"
#define CONFIG "AlexaClientSDKConfig.json"
#define CONFIG_DEFAULT "AlexaClientSDKConfig_default.json"
#define REDIRECT_URI "http://localhost:3000/authresponse"
#define WEBROOT "/etc/alexa/web/"

char config_path[256]={0};
char config_default_path[256]={0};
char redirect_uri[256]={0};
char port[16]={0};
char web_root[256]={0};
int listenfd, clients[CONNMAX];



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

	strcpy(config_path, CONFIG);
	strcpy(config_default_path, CONFIG_DEFAULT);
	strcpy(port, PORT);
	strcpy(redirect_uri, REDIRECT_URI);
	strcpy(web_root, WEBROOT);
	
	//Parsing the command line arguments

	while ((c = getopt (argc, argv, "p:c:r:w:d:")) != -1) {
		switch (c)
		{
			case 'c':
				strcpy(config_path, optarg);
				break;
			case 'p':
				strcpy(port,optarg);
				break;
			case 'r':
				strcpy(redirect_uri,optarg);
				break;
			case 'w':
				strcpy(web_root, optarg);
				break;
			case 'd':
				strcpy(config_default_path, optarg);
				break;
			default:
				printf("\nERROR: Unknown parameter\n");
				exit(-1);
		}
	}

	printf("AVS Auth Server started.\nWebroot:%s%s%s \nPort: %s%s%s \nConfig: %s%s%s \nConfig default: %s%s%s \nRedirect URI: %s%s%s\n",
	"\033[92m",web_root,"\033[0m",
	"\033[92m",port,"\033[0m",
	"\033[92m",config_path,"\033[0m",
	"\033[92m",config_default_path,"\033[0m",
	"\033[92m",redirect_uri,"\033[0m");
	
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
			printf("accept() error");
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

int sendFromDirectory(int client, char *webroot, char *filename)
{
	int fd;
	char full_path[256] ={0};
	char data_to_send[BYTES]={0};
	int nread=0;
	strcat(full_path, webroot);
	strcat(full_path, filename);
	
	printf("\nSend %s\n", full_path);
   
	fd = open(full_path, O_RDONLY);
	if (fd == -1){
		printf("\nFailed to open %s\n", full_path);
		write(client, "HTTP/1.0 404 Not Found\n",23);
		return -1;
	}

	write(client, "HTTP/1.0 200\n",13);
	write(client, "\n", 1);
	while((nread = read(fd, data_to_send, BYTES)) > 0){
		printf("Get %d data\n", nread);
		write(client, data_to_send, nread);
		
	}
	close(fd);

}

int sendRedirect(int client, char *url)
{
	char content[1000];
	sprintf(content, "Location: %s\n",url);
	write(client, "HTTP/1.0 307\n",13);
	write(client, content, strlen(content)+1);
	write(client, "\n", 1);
	return 0;
}

int copyFile(char *from, char *to)
{
	FILE *fpFrom, *fpTo;
	char buffer[4096];
	int bytes;
	fpFrom = fopen(from, "r");
	fpTo = fopen(to, "w+");
	
	printf("\nCopy File from %s to %s\n", from , to);
	
	if(fpFrom == NULL){
		printf("\nFailed to open file %s", from);
		return -1;
	}
	if(fpTo == NULL) {
		printf("\nFailed to open file %s", to);
		fclose(fpFrom);
		return -1;
	}
	while((bytes = fread(buffer, 1, sizeof(buffer), fpFrom)) > 0){
		printf("#####get %d bytes\n", bytes);
		fwrite(buffer, 1, bytes, fpTo);
	}
	fclose(fpFrom);
	fclose(fpTo);
	
}

json_object* get_config(){
	json_object *config = getCleanConfig(config_path);
	if (config == NULL) {
		printf("\nINFO: Failed to open config \"%s\", use default config", config_path);
		config = getCleanConfig(config_default_path);
		if(config == NULL)
		{
			printf("\nERROR: Failed to open default config\"%s\". Exit.", config_default_path);
		}
	}
	return config;
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
				printf("\nRequest: %s", reqline[1]);
			
				json_object *json_config = NULL;
				json_config = get_config();
				if(json_config == NULL)
				{
					printf("\nERROR: Failed to get config. Exit.");
					exit(-1);
				}
				
				if ( strncmp(reqline[1], "/\0", 2)==0 ) {
					
					char * refresh_token = get_config_param_value(json_config, "refreshToken");
					if (refresh_token == NULL || strlen(refresh_token) == 0){
						ret = sendFromDirectory(clients[n], web_root, "splash_screen.html");
					}
					else
					{
						ret = sendFromDirectory(clients[n], web_root, "logout.html");
					}
					
				}else if (strncmp(reqline[1], "/authresponse", 13)==0) {
					ret = handleAuthCodeGrant(clients[n], reqline[1], json_config);
					if (ret == 0) {
						printf("\nGOOD: successfully generated the config with refresh token. Exit.\n");
						ret = sendFromDirectory(clients[n], web_root, "things_to_try.html");
					}
				}else if(strncmp(reqline[1], "/login", 6)==0) {
					ret = handleUserRequest(clients[n], json_config);
				}else if(strncmp(reqline[1], "/logout", 7)==0) {
					copyFile(config_default_path, config_path);
					ret = sendRedirect(clients[n], "/splash_screen.html");
				}
				else{
					ret = sendFromDirectory(clients[n], web_root, reqline[1]);
				}
				json_object_put(json_config);
			}
			

		}
	}

	//Closing SOCKET
	shutdown (clients[n], SHUT_RDWR);         //All further send and recieve operations are DISABLED...
	close(clients[n]);
	clients[n]=-1;
	exit(1);
}
