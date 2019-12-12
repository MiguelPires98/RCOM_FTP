#include "download.h"


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
		puts("Exiting");
		exit(-1);
	}		

    if(strlen(data->user) == 0 || strlen(data->password) == 0){
		puts("Can't parse login info, logging in as anonymous");
		free(data->user);
		free(data->password);
		data->user = malloc(11);
		data->password = malloc(11);
		strncpy(data->user, "anonymous",10);
        strncpy(data->password, "anonymous",10);
        puts(data->user);
	}
	
	
	address = getIP(data->hostname);
	if(address == NULL){
		exit(-1);
	}


	socketfd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);	
	if(socketfd == -1){
		printf("Socket fail\n");
		exit(-1);
	} else{
		printf("Socket Success\n");
	}
	connectstatus = connect(socketfd, address->ai_addr, address->ai_addrlen);
	if(connectstatus == -1){
		printf("connect fail\n");
		exit(-1);
	} else{
		printf("connect success\n");
	}

	//Check for server ready code
	recv(socketfd, reply, BUFFER_SIZE, 0);
	if(strncmp("220", reply, 3)){
        puts(reply);
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

	send(socketfd, "PASV\r\n", 6, 0);
	recv(socketfd, reply, BUFFER_SIZE, 0);
	
	if(strncmp(reply, "227", 3)){
		printf("couldn't pasv\n");
		exit(-1);
	}	
	
	
	PortHelper portH = getPort(reply);
	//printf( "%d.%d.%d.%d", pasvResponse[0], pasvResponse[1], pasvResponse[2], pasvResponse[3]);
	
	char ipaddr[45];
    int i;
    int len=0;
    int count =0;
    for(i=0;i<4;i++){
        len = strlen(portH.port[i]);
        memcpy(ipaddr+count,portH.port[i], len);
        count+=len;
        if(i!=3) ipaddr[count] = '.';
        else ipaddr[count] = 0;
        count++;
    }
    int iport = 0;
    iport =  atoi(portH.port[4]) << 8;
    iport += atoi(portH.port[5]); 
    puts("ip Address and port from pasv:");
    puts(ipaddr);
    printf("%d\n",iport);

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons( iport);
	inet_aton(ipaddr, (struct in_addr*) &addr.sin_addr.s_addr);

	
	int ftpsocketfd = socket(AF_INET, SOCK_STREAM, 0);
	int ftpsocketconnect = connect(ftpsocketfd,(const struct sockaddr*) &addr, sizeof(addr));
	if(ftpsocketconnect == -1){
		puts("pasv connection not made");
	}

	
	char filepath[BUFFER_SIZE];
	int tempsize=0;


    memcpy(filepath, "RETR ",6);

    tempsize = strlen(data->filepath);
    if(tempsize!=0){
        if((tempsize + 8) > BUFFER_SIZE){
            puts("file path too big");
            exit(-1);
        }
        strncat(filepath, data->filepath,BUFFER_SIZE - tempsize);
        //strncat(filepath, "/",1);
        
    }
    tempsize = strlen(data->filename);
    if(tempsize == 0) {
        puts("no filename");
        exit(-1);
    }

    strncat(filepath, data->filename, BUFFER_SIZE - strlen(filepath));

    strncat(filepath,"\r\n",3);

    puts(filepath);
    int counter = 0;
    while(1){
        
        send(socketfd, filepath, strlen(filepath), 0);
        recv(socketfd, reply, BUFFER_SIZE, 0);
        if(strncmp("150", reply, 3)){
            puts("file is unavailable at this time");
            counter ++;
            if(counter == 5){
                puts("File unavailable");
                exit(-1);
            }
            sleep(1);
            continue;
        }
        break;
    }

	char tmpfile[BUFFER_SIZE];
	int read = 1;
	int fd = open(data->filename, O_CREAT|O_WRONLY, 0777);
	while(read){
		read = recv(ftpsocketfd, tmpfile, BUFFER_SIZE, 0);
		write(fd, tmpfile, read);
	}
	close(fd);

	
	return 0;
	
}

//Done - Francisco Correia, este não foi distribuida na na aula porque estava escondida la em cima
struct addrinfo* getIP(char* name){

    struct addrinfo hints, *info;
    struct sockaddr_in *h;
    char straddr[46];


    memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;


	int r = getaddrinfo(name,FTP_PORT_STRING, &hints, &info);
;

	if(r != 0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(r));
		return NULL;
	}
	
	h = (struct sockaddr_in*)info->ai_addr; 
    strcpy(straddr, inet_ntoa(h->sin_addr));

	printf("ip address %s\n", straddr);

	return info;
}

//Done - Francisco Correia
PortHelper getPort(char* reply){
	int i;
	char *tokenizer;
	PortHelper portH;


	tokenizer = strtok(reply, "(,)");

    i=0;
	while((tokenizer = strtok(NULL, "(,)")) != NULL){
		
		strncpy(portH.port[i], tokenizer,4);
		//puts(tokenizer);
        i++;
        if(i == 6)break;
	}
	if(i < 6){
		printf("PASV reply parsing fail\n");
        exit(-1);
	}


		 
	return portH;

}

int login(int sockFd, char *username, char *password){
	char *reply = malloc(BUFFER_SIZE);
	char *userMsg = (char *) malloc(8+strlen(username));
	sprintf(userMsg, "USER %s\r\n", username);
	char *passMsg = (char *) malloc(8+strlen(password));
	sprintf(passMsg, "PASS %s\r\n", password);

    puts(userMsg);
    puts(passMsg);

	int sent = send(sockFd, userMsg, strlen(userMsg), 0);	
	if(sent <= 0)
		return -1;
	recv(sockFd, reply, BUFFER_SIZE, 0);

	//printf("0 - %s\n", reply);

	if(strncmp("331", reply, 3)){
        puts("331 fail");
        puts(reply);
		return -1;
	}

	sent = send(sockFd, passMsg, strlen(passMsg), 0);
	if(sent <= 0)
		return -1;
	recv(sockFd, reply, BUFFER_SIZE, 0);
	
	//printf("1 - %s", reply);

	if(!strncmp("530", reply, 3)){
		return -2;
	}	
	else if(!strncmp("230", reply, 3)){
		free(reply);
		return 0;
	}
	else{
		return -1;	
	}

}

URLInfo* parseURL(const char* url){
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
	URLInfo* data = malloc(sizeof(URLInfo));
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

    puts(data->hostname);
    puts("filepath");
    puts(data->filepath);
    puts("filename");
    puts(data->filename);
	regfree(&regex);
	return data;
	
}



