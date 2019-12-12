#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
//#include <netdb.h>
#include <strings.h>
#include <string.h>
#include <regex.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define FTP_PORT 21
#define FTP_PORT_STRING "21"

#define BUFFER_SIZE 256

#define NO_FILEPATH_TEST "ftp://anonymous:anonymous@speedtest.tele2.net/5MB.zip"

typedef struct URLInfo
{
	char* user; 
	char* password; 
	char* hostname; 
	char* filename; 
	char* filepath;
}URLInfo;


typedef struct PortHelper{
	char port[6][4];
}PortHelper;

struct addrinfo* getIP(char* name);
URLInfo* parseURL(const char* url);
PortHelper getPort(char* ip);
int login(int sockFd, char *username, char *password);




