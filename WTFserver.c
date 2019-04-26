#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h> 
#include <string.h>
#include <sys/socket.h> 
#include <sys/types.h> 
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h> 
#include <fcntl.h>
#define MAX 80
#define SA struct sockaddr 

void create(char* buff){
    char* token = strtok(buff, ";");
    printf("token is %s\n", token);
    token = strtok(NULL, " "); 
    printf("name: %s\n", token);
    DIR* dir = opendir(token);
    if (dir){	//directory exists
    	printf("Project already exists\n");
    }
    else if (ENOENT == errno)	//directory doesn't exist
    {
	mkdir(token, 0700);
	char* path = (char*)malloc(sizeof(char)*(strlen(token)+12));
    	path = strcpy(path, token);
    	path = strcat(path, "/.Manifest");
    	int manifestFile;
    	manifestFile = creat(path, O_WRONLY | 0600);
    	if(manifestFile == -1){
       		printf("cannot create .Manifest\n");
   	}
    	write(manifestFile, "1", 1);
    	close(manifestFile);
    }
    else{
    	printf("Error creating project directory\n");
    }
}

// Function designed for chat between client and server. 
void *func(void* vptr_sockfd){ 
    int sockfd = *((int *) vptr_sockfd);
    char buff[MAX]; 
    int n; 
    // infinite loop for chat 
    for (;;) { 
        bzero(buff, MAX); 
  
        // read the message from client and copy it in buffer 
        read(sockfd, buff, sizeof(buff)); 
        // print buffer which contains the client contents 
        printf("From client: %s\t To client : ", buff);
        create(buff);
        bzero(buff, MAX); 
        n = 0; 
        // copy server message in the buffer 
        while ((buff[n++] = getchar()) != '\n'); 
        // and send that buffer to client 
        write(sockfd, buff, sizeof(buff));
        break;  
    } 
} 
  
// Driver function 
int main(int argc, char *argv[]) 
{ 
    int sockfd, connfd, len; 
    struct sockaddr_in servaddr, cli; 
    // socket create and verification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully created..\n"); 
    bzero(&servaddr, sizeof(servaddr)); 
  
    //check if port # is valid
    int i;
    char* port = argv[1];
    printf("port is: %s\n", port);
    for(i = 0; i<strlen(port); i++){
      //printf("current char: %c\n", port[i]);
      if(!isdigit(port[i])){
	//printf("Invalid port number\n");
	exit(0);
      }
    }
    printf("Valid port number used....\n");
    int num = atoi(port);
    printf("i am the num converted port: %i\n", num);
    
    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(num);
  
    // Binding newly created socket to given IP and verification 
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
        printf("socket bind failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully binded..\n"); 
  
    // Now server is ready to listen and verification 
    if ((listen(sockfd, 5)) != 0) { 
        printf("Listen failed...\n"); 
        exit(0); 
    } 
    else
        printf("Server listening..\n"); 
    len = sizeof(cli); 
  
    // Accept the data packet from client and verification 
    connfd = accept(sockfd, (SA*)&cli, &len); 

    while(!(connfd < 0)){
	  printf("server accept the client...\n");
	  //not sure if this works right here (need to test), thread also creates warning when compiled but works
	  //also need to make a struct to keep track of threads to close them properly when reaching SIGINT(CTRL+C)
	  pthread_t thread_id;
	  pthread_create(&thread_id, NULL, func, (void*)&connfd);
	  pthread_join(thread_id, NULL);
	  printf("thread done\n");
	  connfd = accept(sockfd, (SA*)&cli, &len);
    }
    
    if (connfd < 0) { 
	  printf("server accept failed...\n"); 
	  exit(0); 
    } 
    // Function for chatting between client and server 
    //func(connfd); 
    printf("server closing\n");

    // After chatting close the socket 
    close(sockfd); 
} 
