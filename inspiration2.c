#include "download2.h"

struct addrinfo* getIP(char* name){
	struct addrinfo hints;
	memset(&hints, 0, sizeof hints);
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
	char* straddr = malloc(100);
	inet_ntop(AF_INET, addr, straddr, 100);
	fprintf(stdout, "Address translated to ip %s\n", straddr);
	free(straddr);
	return info;
}



int main(int argc, char** argv){
	if (argc != 2){
		printf("Usage: %s ftp://[username:password]@site/path/to/file\n", argv[0]);
		exit(-1);
	}
	
	URLData *data = parseURL(argv[1]);
	if(data == NULL){
		puts("Exiting");
		exit(-1);
	}		
	
	
	struct addrinfo *address = getIP(data->hostname);
	if(address == NULL){
		exit(-1);
	}


	int socketfd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);	
	if(socketfd == -1){
		perror("couldn't open socket, aborting:");
		exit(-1);
	}
	int connectstatus = connect(socketfd, address->ai_addr, address->ai_addrlen);
	if(connectstatus == -1){
		perror("couldn't connect to server, aborting:");
		exit(-1);
	}
	else
		puts("connection successfully made");

	char *responses = malloc(1000);
	recv(socketfd, responses, 1000, 0);
	if(strncmp("220 ", responses, 4)){
		printf("No 220 code received");
		exit(-1);
	}


	int st;
	if(strlen(data->user) == 0 || strlen(data->password) == 0){
		printf("No credentials provided. Logging in with default credentials:\nusername: anonymous\npassword: not.an@email.com\n");
			st = login(socketfd, "anonymous", "not.an@email.com");	
	}	
	else{
		st = login(socketfd, data->user, data->password);	
	}
	
	if(st == 0){
		printf("Login successful\n");	
	}
	else if(st == -1){
		printf("Error\n");
		exit(-1);
	}
	else if(st == -2){
		printf("Login incorrect\n");
		exit(-2);
	}

	send(socketfd, "PASV\r\n", 6, 0);
	recv(socketfd, responses, 1000, 0);
	
	if(strncmp(responses, "227 ", 4)){
		printf("couldn't pasv\n");
		exit(-1);
	}	
	
	
	int *pasvResponse = getPort(responses);
	//printf( "%d.%d.%d.%d", pasvResponse[0], pasvResponse[1], pasvResponse[2], pasvResponse[3]);
	
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
	recv(socketfd, responses, 1000, 0);
	if(strncmp("150 ", responses, 4)){
		puts("file is unavailable at this time");
		return -1;
	}

	char tmpfile[1000];
	int bytesread;
	int fd = open(data->filename, O_CREAT|O_TRUNC|O_WRONLY, 0777);
	do{
		bytesread = recv(secondsocketfd, tmpfile, 1000, 0);
		write(fd, tmpfile, bytesread);
	}while(bytesread);
	close(fd);
	
	free(responses);
	
	return 0;
	
}

int *getPort(char* ip){
	//puts(ip);
	int i = 0;
	char *dump = malloc(100);
	char port[16][14]; //stack smashing prevention 
	dump = strtok(ip, "(,)");
	while((dump = strtok(NULL, "(,)")) != NULL){
		
		strcpy(port[i++], dump);
		//puts(dump);
	}
	if(i < 6){
		printf("Address parsing failed. Exiting.\n");
	}


	/*for(i = 0; i < 6; i++){
		puts(port[i]);
	}*/
	
	int *pasvResponse = malloc(6*sizeof(int));
	for(i = 0; i < 6; i++){
		pasvResponse[i] = atoi(port[i]);
		//printf("i - %d\n", pasvResponse[i]);
	}
		 
	return pasvResponse;

}

int login(int sockFd, char *username, char *password){
	char *responses = malloc(1000);
	char *userMsg = (char *) malloc(8+strlen(username));
	sprintf(userMsg, "USER %s\r\n", username);
	char *passMsg = (char *) malloc(8+strlen(password));
	sprintf(passMsg, "PASS %s\r\n", password);

	int sent = send(sockFd, userMsg, strlen(userMsg), 0);	
	if(sent <= 0)
		return -1;
	recv(sockFd, responses, 1000, 0);

	//printf("0 - %s\n", responses);

	if(strncmp("331 ", responses, 4)){
		return -1;
	}

	sent = send(sockFd, passMsg, strlen(passMsg), 0);
	if(sent <= 0)
		return -1;
	recv(sockFd, responses, 1000, 0);
	
	//printf("1 - %s", responses);

	if(!strncmp("530 ", responses, 4)){
		return -2;
	}	
	else if(!strncmp("230 ", responses, 4)){
		free(responses);
		return 0;
	}
	else{
		return -1;	
	}

}

URLData* parseURL(const char* url){
	char* regexp = "ftp://(([^:]*):([^@]*)@)?([^/]+)/(.*/)?([^/]*)"; 
	regex_t regex;
	int r =	regcomp(&regex, regexp, REG_EXTENDED);
	if (r != 0){
		perror("regex not compiled:");
		return NULL;
	}
	regmatch_t pmatch[7];
	r = regexec(&regex, url, 7, pmatch, 0);
	if (r == REG_NOMATCH){
		perror("regex not matches:");
		return NULL;
	}
	//grupo 0 1 6 pra descartar	
	URLData* data = malloc(sizeof(URLData));
	data->user = malloc(1+pmatch[2].rm_eo-pmatch[2].rm_so);
	data->password = malloc(1+pmatch[3].rm_eo-pmatch[3].rm_so);
	data->hostname = malloc(1+pmatch[4].rm_eo-pmatch[4].rm_so);
	data->filepath = malloc(1+pmatch[5].rm_eo-pmatch[5].rm_so);
	data->filename = malloc(1+pmatch[6].rm_eo-pmatch[6].rm_so);
	strncpy(data->user, url+pmatch[2].rm_so, pmatch[2].rm_eo-pmatch[2].rm_so);
	strncpy(data->password, url+pmatch[3].rm_so, pmatch[3].rm_eo-pmatch[3].rm_so);
	strncpy(data->hostname, url+pmatch[4].rm_so, pmatch[4].rm_eo-pmatch[4].rm_so);
	strncpy(data->filepath, url+pmatch[5].rm_so, pmatch[5].rm_eo-pmatch[5].rm_so);
	strncpy(data->filename, url+pmatch[6].rm_so, pmatch[6].rm_eo-pmatch[6].rm_so);
	data->user[pmatch[2].rm_eo-pmatch[2].rm_so] = 0;
	data->password[pmatch[3].rm_eo-pmatch[3].rm_so] = 0;
	data->hostname[pmatch[4].rm_eo-pmatch[4].rm_so] = 0;
	data->filepath[pmatch[5].rm_eo-pmatch[5].rm_so] = 0;
	data->filename[pmatch[6].rm_eo-pmatch[6].rm_so] = 0;

	regfree(&regex);
	return data;
	
}



