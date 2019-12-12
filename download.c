#include "download.h"

struct addrinfo* getIP(char* name){
	
	
	struct addrinfo hints; 
	//we are only interested in IPv4 address of sock_stream socket type
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	struct addrinfo *info; 
	memset(&info, 0, sizeof hints);
	char port[3]; 
	sprintf(port, "%d", FTP_PORT);
	int r = getaddrinfo(name,port, &hints, &info);

	if(r != 0){
		fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(r));
		return NULL;
	}
	
	struct sockaddr_in *ip = (struct sockaddr_in*)info->ai_addr; 
	struct in_addr *addr = &(ip->sin_addr);
	char straddr[100];
	inet_ntop(AF_INET, addr, straddr, 100);
	fprintf(stdout, "Address translated to ip %s\n", straddr);
	free(straddr);
	return info;
}



int main(int argc, char** argv){


	char reply[BUFFER_SIZE];
	URLInfo *data;
	struct addrinfo *address;
	int socketfd, connectstatus,st;

	if (argc != 2){
		printf("Usage: %s ftp://[username:password]@site/path/to/file\n", argv[0]);
		exit(-1);
	}
	
	data = parseURL(argv[1]);
	if(data == NULL){
		printf("Exiting");
		exit(-1);
	}		

	if(strlen(data->user) == 0 || strlen(data->password) == 0){
		puts("Can't parse login info, logging in as anonymous");
		free(data->user);
		free(data->password);
		data->user = malloc(10);
		data->password = malloc(1);
		strncpy(data->user, "anonymous",10);
	}

	
	
	
	address = getIP(data->hostname);
	if(address == NULL){
		exit(-1);
	}


	socketfd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);	
	if(socketfd == -1){
		printf("Socket fail");
		exit(-1);
	} else{
		printf("Socket Success");
	}
	connectstatus = connect(socketfd, address->ai_addr, address->ai_addrlen);
	if(connectstatus == -1){
		printf("connect fail");
		exit(-1);
	} else{
		printf("connect success");
	}


	//Check for server ready code
	recv(socketfd, reply, BUFFER_SIZE, 0);
	if(strncmp("220 ", reply, 4)){
		printf("Server is not ready");
		exit(-1);
	}

	st = login(socketfd, data->user, data->password);	
	
	
	if(st == 0){
		printf("Login successful\n");	
	}
	else if(st == -1){
		printf("Login Error\n");
		exit(-1);
	}
	else if(st == -2){
		printf("Login incorrect\n");
		exit(-2);
	} else{
		exit(-3);
	}


	//SETUP PASV
	send(socketfd, "PASV\r\n", 6, 0);
	recv(socketfd, reply, BUFFER_SIZE, 0);
	
	if(strncmp(reply, "227 ", 4)){
		printf("couldn't pasv\n");
		exit(-1);
	}	
	
	
	int *pasvResponse = getPort(reply);
	
	
	char ipaddr[21];
	sprintf(ipaddr,  "%d.%d.%d.%d", pasvResponse[0], pasvResponse[1], pasvResponse[2], pasvResponse[3]);

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons( (pasvResponse[4] << 8) + pasvResponse[5]);
	inet_aton(ipaddr, (struct in_addr*) &addr.sin_addr.s_addr);

	
	int secondsocketfd = socket(AF_INET, SOCK_STREAM, 0);
	int secondsocketconnect = connect(secondsocketfd,(const struct sockaddr*) &addr, sizeof(addr));
	if(secondsocketconnect == -1){
		puts("pasv connection not made");
	}

	
	char fileretr[400];
	int lengthfileretr;
	if(strlen(data->filepath) != 0)
		lengthfileretr = snprintf(fileretr, 400, "RETR %s/%s\r\n", data->filepath, data->filename);	
	else
		lengthfileretr = snprintf(fileretr, 400, "RETR %s\r\n", data->filename);	
	
	send(socketfd, fileretr, lengthfileretr, 0);
	recv(socketfd, responses, BUFFER_SIZE, 0);
	if(strncmp("150 ", responses, 4)){
		puts("file is unavailable at this time");
		return -1;
	}

	char tmpfile[BUFFER_SIZE];
	int bytesread;
	int fd = open(data->filename, O_CREAT|O_TRUNC|O_WRONLY, 0777);
	do{
		bytesread = recv(secondsocketfd, tmpfile, BUFFER_SIZE, 0);
		write(fd, tmpfile, bytesread);
	}while(bytesread);
	close(fd);
	
	free(responses);
	
	return 0;
	
}

int *getPort(char* ip){


}

int login(int sockFd, char *username, char *password){


}

URLInfo* parseURL(const char* url){

	
}



