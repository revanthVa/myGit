#include <netdb.h> 
#include <netinet/in.h> 
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
#define PORT 8726
#define SA struct sockaddr 

typedef struct manifestData{
    int numberOfFiles;
    struct fileData* file;
}manifestData;

typedef struct fileList{	//linked list of file data
	int length;
	int size;
	char* name;
	struct fileData* next;
}fileData;

manifestData* md = NULL;

void getFileData(char* file){
	manifestData* mdPtr = md;
}
void createManifestData(char* project){
	char* path = (char*)malloc(sizeof(char)*(strlen(project)+12));
	path = strcpy(path, project);
	path = strcat(path, "/.Manifest");
	int manifestFile;
	manifestFile = open(path, O_RDONLY);
	if(manifestFile == -1){
		printf("cannot open .Manifest\n");
	}
	md = (manifestData*)malloc(sizeof(manifestData));
	md->numberOfFiles = 1;
	int currentPos = lseek(manifestFile, 0, SEEK_CUR);
	int size = lseek(manifestFile, 0, SEEK_END);    //get length of file
	lseek(manifestFile, currentPos, SEEK_SET);  //set position back to start
	char c[size+1];
	int tracker = 0;
	int linesize = 0;
	if(read(manifestFile, c, size) != 0){
		while (tracker < size){
			linesize = 0;
			while (c[tracker] != '\n'){
				tracker++;
				linesize++;
			}
			if (linesize <2){ //skips first line
				tracker++;
				continue;	//line doces not have contain a filename
			}
			char* line = (char*)malloc(sizeof(char)*linesize+1);
			memcpy(line, &c[tracker-linesize], linesize);
			line[strlen(line)] = '\0';
			printf("line is %s\n", line);
			int numPosition = tracker+2-linesize;
			int numSize = 0;
			while (c[numPosition] != ' '){	//gets length of version
				numPosition++;
				numSize++;
				if (numSize > 500){
					printf("error\n");
					exit(1);
				}
			}
			char* version = (char*)malloc(sizeof(char)*(numSize+1));
			version[strlen(version)] = '\0';
			memcpy(version, &c[tracker+2-linesize], numSize);
			printf("version is %s\n", version);
			int fileNameSize = linesize-44-strlen(version)+1; //with null teminator
			char* fileName = (char*)malloc(sizeof(char)*fileNameSize);
			memcpy(fileName, &c[(tracker-linesize)+3+strlen(version)], fileNameSize-1);
			fileName[strlen(fileName)] = '\0';
			//printf("version length %i\n", strlen(version));
			printf("fileName is %s\n", fileName);
			getFileData(fileName);
		}
	}
	close(manifestFile);
}

void create(char* token){
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

void checkout(char* token){
    printf("token is %s\n", token);
    token = strtok(NULL, " "); 
    printf("name: %s\n", token);
    DIR* dir = opendir(token);
    if (dir){	//directory exists
    	printf("Project exists\n");
    }
    else if (ENOENT == errno)	//directory doesn't exist
    {
        printf("Project does not exist\n");
        exit(1);
    }
}
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
	    char* token = strtok(buff, ":");
	    if (strcmp(token, "create") == 0){
        	create(token);
	    }
        else if (strcmp(token, "checkout") == 0){
            printf("checkout\n");
            checkout(token);
        }
        bzero(buff, MAX); 
        n = 0; 
        // copy server message in the buffer 
        //while ((buff[n++] = getchar()) != '\n'); 
        // and send that buffer to client 
        //write(sockfd, buff, sizeof(buff));
        break;  
    } 
} 
  
// Driver function 
int main(int argc, char *argv[]) 
{ 
    createManifestData("test");
    /*
    if (argc!= 2){
        printf("Incorrect number of arguments. Enter a port number\n");
        exit(1);
    }
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
    */
} 
