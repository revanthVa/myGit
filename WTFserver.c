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

//gcc WTFserver.c -lpthread -o WTFserver

typedef struct manifestData{
    int numberOfFiles;
    struct fileList* file;
}manifestData;

typedef struct fileList{	//linked list of file data
	int length;
	int size;
	char* name;
	char* data;
	struct fileList* next;
}fileList;

manifestData* md = NULL;
fileList* fl = NULL;

void getFileData(char* fileName){
	manifestData* mdPtr = md;
	fileList* flPtr = fl;
	while (flPtr->next != NULL){
		flPtr = flPtr->next;
	}
	fileList* tmp = (fileList*)malloc(sizeof(fileList));
	tmp->length = strlen(fileName);
	tmp->name = (char*)malloc(sizeof(char)*strlen(fileName)+1);
	memcpy(tmp->name, fileName, strlen(fileName));
	tmp->name[strlen(fileName)] = '\0';
	//printf("tmp file length is %i\n", tmp->length);
	//printf("tmp file name is %s\n", tmp->name);
	int fileptr = open(fileName, O_RDONLY);
	if(fileptr == -1){
		printf("cannot open file\n");
		exit(1);
	}
	int currentPos = lseek(fileptr, 0, SEEK_CUR);
	int size = lseek(fileptr, 0, SEEK_END);    //get length of file
	lseek(fileptr, currentPos, SEEK_SET); 	//set position back to start
	tmp->size = size;
	//printf("first file size is %i\n", tmp->size);
	char c[size+1];
	c[size] = '\0';
	if(read(fileptr, c, size) != 0){
		tmp->data = (char*)malloc(sizeof(char)*size+1);
		strcpy(tmp->data, c);
	}
	printf("file has this data: \n%s", tmp->data);
	close(fileptr);
	flPtr->next = tmp;
}

void createManifestData(char* project){
	char* path = (char*)malloc(sizeof(char)*(strlen(project)+12));
	path = strcpy(path, project);
	path = strcat(path, "/.Manifest");
	int manifestFile;
	manifestFile = open(path, O_RDONLY);
	if(manifestFile == -1){
		printf("cannot open .Manifest\n");
		exit(1);
	}
	md = (manifestData*)malloc(sizeof(manifestData));
	fl = (fileList*)malloc(sizeof(fileList));
	md->file = fl;
	md->numberOfFiles = 1;
	fl->length = strlen(path);
	fl->name = (char*)malloc(sizeof(char)*strlen(path)+1);
	memcpy(fl->name, path, strlen(path));
	fl->name[strlen(path)] = '\0';
	printf("manifest length is %i\n", md->file->length);
	printf("manifest name is %s\n", md->file->name);
	int currentPos = lseek(manifestFile, 0, SEEK_CUR);
	int size = lseek(manifestFile, 0, SEEK_END);    //get length of file
	lseek(manifestFile, currentPos, SEEK_SET);  //set position back to start
	md->file->size = size;
	printf("manifest size is %i\n", md->file->size);
	
	char c[size+1];
	c[size] = '\0';
	int tracker = 0;
	int linesize = 0;
	if(read(manifestFile, c, size) != 0){
	md->file->data = (char*)malloc(sizeof(char)*size+1);
	strcpy(md->file->data, c);
	//printf("manifest has this data: %s\n", md->file->data);
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
			//printf("line is %s\n", line);
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
			//printf("version is %s\n", version);
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

void DeleteAll(char* pathorfile){ //implements recursive function to delete all files and subdirectories of server folder
   printf("Current path: %s\n", pathorfile);
   struct dirent *pDirent;
   DIR *pDir;
   pDir = opendir(pathorfile);
   if(pDir == NULL){
       printf("Cannot open directory '%s'\n", pathorfile);
       return;
   }
   while ((pDirent = readdir(pDir)) != NULL) {
       char* nextpathorfile = (char*)malloc(sizeof(char)*(strlen(pDirent->d_name)));
       strcpy(nextpathorfile,pDirent->d_name);
       int newsize = strlen(pathorfile) + strlen(nextpathorfile) + 1;
       char *new = (char*)malloc(sizeof(char)*(newsize));
       strcpy(new, pathorfile);
       if(new[strlen(new)-1] != '/'){
           strcat(new, "/");
       }
       strcat(new, nextpathorfile);
       if(strcmp(nextpathorfile, ".") == 0 || strcmp(nextpathorfile, "..") == 0){
           continue;
       }
       struct stat path_stat;
       stat(new, &path_stat);
       if(S_ISDIR(path_stat.st_mode)){ //is a directory
           printf("Directory: %s\n", new);
           DeleteAll(new);
       }
       else if(S_ISREG(path_stat.st_mode)){ //is a file
           printf("File: %s\n", new);
	   remove(new);
       }
   }
   closedir(pDir);
   rmdir(pathorfile);
   return;
}

void create(char* token){
    printf("token is %s\n", token);
    token = strtok(NULL, " "); 
    printf("name: %s\n", token);
    DIR* dir = opendir(token);
    if (dir){	//directory exists
    	printf("Project already exists. Create failed.\n");
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
    	printf("Error creating project directory.\n");
    }
}

void destroy(char* token){ //need to edit later on how to expire pending commits
    //printf("token is %s\n", token);
    token = strtok(NULL, " "); 
    //printf("name: %s\n", token);
    DIR* dir = opendir(token);
    if (dir){	//directory exists
	//chmod(token, 00040); //lock the directory first?
      //"You have to use an independent mutex lock for each project. That way only one thread can access each project."
		DeleteAll(token);
    }
    else if (ENOENT == errno)	//directory doesn't exist
    {
		printf("Project does not exist. Destroy failed.\n");
    }
    else{
    	printf("Error destroying project directory.\n");
    }
}

void checkout(char* token, int sockfd){
    //printf("token is %s\n", token);
    token = strtok(NULL, " "); 
    printf("name: %s\n", token);
    DIR* dir = opendir(token);
    if (dir){	//directory exists
    	printf("Project exists\n");
    	char* systemStr = (char*)malloc(sizeof(char)*14+(strlen(token)*2+2));
    	strcpy(systemStr, "tar cfz ");
    	strcat(systemStr, token);
    	strcat(systemStr, ".tgz ");
    	strcat(systemStr, token);
    	system(systemStr);
    	char* tarFile = (char*)malloc(sizeof(char)*(strlen(token)+5));
    	strcpy(tarFile, token);
    	strcat(tarFile, ".tgz");
    	tarFile[strlen(token)+4] = '\0';
    	printf("tarFile is %s\n", tarFile);
    	int fileptr = open(tarFile, O_RDONLY);
		if(fileptr == -1){
			printf("cannot open .Manifest\n");
			exit(1);
		}
		int currentPos = lseek(fileptr, 0, SEEK_CUR);
		int size = lseek(fileptr, 0, SEEK_END);    //get length of file
		lseek(fileptr, currentPos, SEEK_SET);  //set position back to start
		char c[size+1];
		c[size+1] = '\0';
		//printf("size is %i\n", size);
		int tracker = 0;
		int linesize = 0;
		int new = 0;
		//new = creat("work.tgz", O_APPEND | O_RDWR | 0600);
		if(read(fileptr, c, size) != 0){
			//while (tracker < size){
				//printf("%c", c[tracker]);
				//tracker++;
			//}
			char sendtar[size+1];
			memcpy(sendtar, c, size);
			sendtar[size] = '\0';
			char sendSize[40];
			sprintf(sendSize, "%d", size);
			//write(sockfd, sendSize, strlen(sendSize));
			//printf("send tar is this \n%s", sendtar);
			//new = creat("work.tgz", O_APPEND | O_RDWR | 0600);
			write(sockfd, sendtar, size);
			remove(tarFile);
    	}
    }
    else if (ENOENT == errno)	//directory doesn't exist
    {
        printf("Project does not exist. Checkout failed.\n");
        exit(1);
    }
}

void update(char* token, int sockfd){	//send manifest to client
    token = strtok(NULL, " "); 
    //printf("name: %s\n", token);
    DIR* dir = opendir(token);
    if (dir){	//directory exists
		char* path = (char*)malloc(sizeof(char)*(strlen(token)+12));
		strcpy(path, token);
		strcat(path, "/.Manifest");
		int manifestFile;
		manifestFile = open(path, O_RDONLY);
		if(manifestFile == -1){
			printf("cannot open .Manifest\n");
			write(sockfd, "exit", 5);
		}
		else{
			int currentPos = lseek(manifestFile, 0, SEEK_CUR);
			int size = lseek(manifestFile, 0, SEEK_END);    //get length of file
			lseek(manifestFile, currentPos, SEEK_SET);  //set position back to start
			char c[size+1];
			c[size+1] = '\0';
			//printf("size is %i\n", size);f
			int tracker = 0;
			int linesize = 0;
			if(read(manifestFile, c, size) != 0){
				//printf("reading manifest\n");
				write(sockfd, c, size);
			}
		}
    }
    else if (ENOENT == errno)	//directory doesn't exist
    {
        printf("Project does not exist. Update failed.\n");
        write(sockfd, "exit", 5);
    }
}

void commit(char* token, int sockfd){
	//printf("token is %s\n", token);
	token = strtok(NULL, " "); 
	//printf("name: %s\n", token);
	DIR* dir = opendir(token);
	if (dir){	//directory exists
	}
	else if (ENOENT == errno){	//directory doesn't exist
	    printf("Project does not exist. Commit failed.\n");
	    write(sockfd, "exit", 5);
	}
}

void currentversion(char* token, int sockfd){
    token = strtok(NULL, " "); 
    //printf("name: %s\n", token);
    DIR* dir = opendir(token);
    if (dir){	//directory exists
	  char* path = (char*)malloc(sizeof(char)*(strlen(token)+12));
	  strcpy(path, token);
	  strcat(path, "/.Manifest");
	  int manifestFile;
	  manifestFile = open(path, O_RDONLY);
	  if(manifestFile == -1){
	    printf("cannot open .Manifest\n");
	    write(sockfd, "exit", 5);
	  }
	  int currentPos = lseek(manifestFile, 0, SEEK_CUR);
	  int size = lseek(manifestFile, 0, SEEK_END);    //get length of file
	  lseek(manifestFile, currentPos, SEEK_SET);  //set position back to start
	  char c[size+1];
	  c[size+1] = '\0';
	  //printf("size is %i\n", size);
	  int tracker = 0;
	  int linesize = 0;
	  if(read(manifestFile, c, size) != 0){
	    //printf("reading manifest\n");
	    write(sockfd, c, size);
	  }
    }
    else if (ENOENT == errno){	//directory doesn't exist
	  printf("Project does not exist. Currentversion command failed.\n");
	  write(sockfd, "exit", 5);
    }
}

void upgrade(char* token, int sockfd, char* copy){
	//printf("1st word: %s\n", token);
	int l = 0;
	int findProjectName = 0;
	int i = 0;
	char* projectName = (char*)malloc(sizeof(char)*20);
	int tracker = 0;
	int dirExists = 1;	//0 = false 1 = true
	char* projectDir = malloc(10);
	for (i = 8; i < strlen(copy); i++){	//upgrade:projectName:
		while (copy[i] != ':'){
			printf("copy[i] %c ", copy[i]);
			tracker++;
			i++;
		}
		projectDir = realloc(projectDir, (sizeof(char)*tracker+1));
		memcpy(projectDir, &copy[i-tracker], tracker);
		projectDir[tracker] = '\0';
		printf("projectDir is %s\n", projectDir);
		DIR* dir = opendir(projectDir);
   	    if (dir){	//directory exists
   	    	dirExists = 1;
   	    }
   	    else if (ENOENT == errno){	//directory doesn't exist
   	    	dirExists = 0;
       		printf("Project does not exist. upgrade failed.\n");
      		write(sockfd, "exit", 5);
    	}
		break;
	}
	
	if (dirExists == 1){
		char* createTar = (char*)malloc(sizeof(char)*strlen(projectDir)+5);
		strcpy(createTar, projectDir);
		strcat(createTar, ".tgz");
		printf("createTar is %s\n", createTar);
		i = 0;
		for (i = 0; i < strlen(copy); i++){	//gets length of all filenames with space
			if (copy[i] == ':'){
				l = l + 2;
			}
			else{
				l++;
			}
		}
		l = l -9;	//subtract unecessary tokens from copy
		char* tarFiles = (char*)malloc(sizeof(char)*strlen(projectName)+15+l);
		
		while ((token = strtok(NULL, ":")) != NULL){
			if (findProjectName == 0){		//first token will be projectName
				int projectlength = strlen(token);
				projectName = (char*)realloc(projectName, (sizeof(char)*projectlength+1));
				strcpy(projectName, token);
				projectName[projectlength] = '\0';
				printf("projectName is %s\n", projectName);
				strcpy(tarFiles, "tar cfz ");
				strcat(tarFiles, projectName);
				strcat(tarFiles, ".tgz");
				strcat(tarFiles, " ");
				findProjectName = 1;
			}
			else{
				strcat(tarFiles, token);
				strcat(tarFiles," ");
				//printf("Next: %s\n", token);
			}
		}
		system(tarFiles);
		printf("tar files is %s\n", tarFiles);
		printf("l is %i\n", l);
    	int fileptr = open(createTar, O_RDONLY);
		if(fileptr == -1){
			printf("cannot open tar file\n");
			exit(1);
		}
		int currentPos = lseek(fileptr, 0, SEEK_CUR);
		int size = lseek(fileptr, 0, SEEK_END);    //get length of file
		lseek(fileptr, currentPos, SEEK_SET);  //set position back to start
		char c[size+1];
		c[size+1] = '\0';
		int tracker = 0;
		int linesize = 0;
		int new = 0;
		if(read(fileptr, c, size) != 0){
			char sendtar[size+1];
			memcpy(sendtar, c, size);
			sendtar[size] = '\0';
			write(sockfd, sendtar, size);
    	}
		remove(createTar);
		free(tarFiles);
		free(createTar);
	}
}

// Function designed for chat between client and server. 
void *func(void* vptr_sockfd){ 
    int sockfd = *((int *) vptr_sockfd);
    char buff[MAX]; 
    int n; 
    bzero(buff, MAX); 
    
    // read the message from client and copy it in buffer 
    read(sockfd, buff, sizeof(buff)); 
    // print buffer which contains the client contents 
    printf("From client: %s\t To client: ", buff);
    char* copy = (char*)malloc(sizeof(char)*strlen(buff)+1);
    strcpy(copy, buff);
    copy[strlen(buff)] = '\0';
    char* token = strtok(buff, ":");
    printf("token is %s\n", token);
    if (strcmp(token, "create") == 0){
    	printf("Performing create command....\n");
    	create(token);
    }
    else if (strcmp(token, "destroy") == 0){
		printf("Performing destroy command....\n");
		destroy(token);
    }
    else if (strcmp(token, "checkout") == 0){
    	printf("Performing checkout command....\n");
    	checkout(token, sockfd);
    }
    else if(strcmp(token, "currentversion") == 0){
    	printf("performing currentversion command....\n");
    	currentversion(token, sockfd);
    }
    else if (strcmp(token, "update") == 0){
    	printf("Performing update command....\n");
    	update(token, sockfd);
    }
    else if (strcmp(token, "upgrade") == 0){
    	printf("Performing upgrade command....\n");
    	upgrade(token, sockfd, copy);
    }
    else if(strcmp(token, "commit") == 0){
		printf("Performing commit command.....\n");
		commit(token, sockfd);
    }else{
    	printf("Invalid command reached towards server. Exiting.\n");
    }
    bzero(buff, MAX); 
    n = 0; 
    // copy server message in the buffer 
    //while ((buff[n++] = getchar()) != '\n'); 
    // and send that buffer to client 
    //write(sockfd, buff, sizeof(buff));
} 
  
// Driver function 
int main(int argc, char *argv[]) 
{ 
	//checkout("test");
    //createManifestData("test");

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
	printf("Invalid port number\n");
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
        printf("Server listening...\n"); 
    len = sizeof(cli); 
  
    // Accept the data packet from client and verification 
    connfd = accept(sockfd, (SA*)&cli, &len); 

    while(!(connfd < 0)){
	  printf("Server accepted the client...\n");
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
    return 0;
} 
