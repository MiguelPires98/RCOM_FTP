#include <sys/types.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <strings.h>
#include <string.h>
#include <regex.h>
#include <fcntl.h>
#include <sys/stat.h>
#define FTP_PORT 21

typedef struct _URLData
{
	char* user; 
	char* password; 
	char* hostname; 
	char* filename; 
	char* filepath;
}URLData;


struct addrinfo* getIP(char* name);
URLData* parseURL(const char* url);
int *getPort(char* ip);
int login(int sockFd, char *username, char *password);




