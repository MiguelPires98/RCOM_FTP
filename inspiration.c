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

//Done - Roberto Lopes
URLInfo* parseURL(const char* parse_url) {

		URLInfo* url = malloc(sizeof(URLInfo));

	url->filename = malloc(BUFFER_SIZE);
	url->filepath = malloc(BUFFER_SIZE);
	url->hostname = malloc(BUFFER_SIZE);
	url->password = malloc(BUFFER_SIZE);
	url->user = malloc(BUFFER_SIZE);

    
	stateMachineUrl(parse_url, url);


	printf("filename %s  size: %ld\n", url->filename, strlen(url->filename));
	printf("filepath %s  size: %ld\n", url->filepath, strlen(url->filepath));
	printf("hostname %s  size: %ld\n", url->hostname, strlen(url->hostname));
	printf("password %s  size: %ld\n", url->password, strlen(url->password));
	printf("user %s  size: %ld\n", url->user, strlen(url->user));

    return url;
}

int stateMachineUrl(const char *parse_url, URLInfo *url) {
    char protocol[] = "ftp://";

	char tempBuffer[BUFFER_SIZE];

    int i = 0;

    //read protocol
    for (i = 0; i < sizeof(protocol) - 1; i++)
    {

        if (protocol[i] != parse_url[i])
        {
            return 1;
        }
    }

	
    unsigned int step = 0;
    unsigned int indice = 0; //indice que vai ser copiado

    // if (!login_mode)
    // {
    //     step = 2;
    // }
    // else
    // {
    //     i++; //to ignore '['
    // }

    while (i < strlen(parse_url))
    {
        switch (step)
        {

        //reading a username
        case (0):
            if (parse_url[i] == ':')
            {
				tempBuffer[indice] = 0;
				strncpy(url->user, tempBuffer,BUFFER_SIZE);
                step = 1;
                indice = 0;
            }
			else if(parse_url[i] == '@'){
				tempBuffer[indice] = 0;
				strncpy(url->user, tempBuffer,BUFFER_SIZE);
				step = 2;
				indice = 0;
				url->password[0] = 0;
			} else if(parse_url[i] == '/'){
				tempBuffer[indice] = 0;
				strncpy(url->hostname, tempBuffer,BUFFER_SIZE);
				step=3;
				indice = 0;
				url->user[0] = 0;
				url->password[0] = 0;
			}
            else
            {
				tempBuffer[indice] = parse_url[i];
                indice++;
            }
            break;

        //reading a password
        case (1):
            if (parse_url[i] == '@')
            {
				url->password[indice] = 0;
                step = 2;
                indice = 0;
            }
            else
            {
                url->password[indice] = parse_url[i];
                indice++;
            }
            break;

        //reading a host
        case (2):
            if (parse_url[i] == '/')
            {
				url->hostname[indice] = 0;
                step = 3;
                indice = 0;
            }
            else
            {
                url->hostname[indice] = parse_url[i];
                indice++;
            }
            break;

        //reading a url_path
        case (3):
            url->filepath[indice] = parse_url[i];
            indice++;
            break;

        default:
            break;
        }
		if (indice == BUFFER_SIZE - 1){
			puts("Path to big");
			exit(-1);

		} 
        i++; //next array index
    }
	url->filepath[indice] = 0;

	
    char *path = (char*)malloc(strlen(url->filepath));
    char *pathtemp = malloc(strlen(url->filepath));
    pathtemp = url->filepath;
    unsigned int cnt = 0;
		int j = 0;
    while(cnt < strlen(url->filepath)){
        if(strchr(pathtemp, '/') != NULL){
            path[cnt] = url->filepath[cnt];
        }
        else{
					url->filename[j] = url->filepath[cnt];
				 	j++;
        }
        pathtemp++;
        cnt++;
    }

		path[strlen(path)] = 0; //erase last char from string
    strcpy(url->filepath, path);

		return 0;
}



