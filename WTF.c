#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <netdb.h>
#include <sys/socket.h> 
#include <dirent.h>
#include <errno.h>
#include <openssl/sha.h>
#include <sys/ioctl.h>
#define MAX 80 

//gcc -o WTF WTF.c -lssl -lcrypto
typedef struct manifestData{
	char* fileName;
	char* hash;
	char flag[2]; // U M A D
	int version;
	struct manifestData* next;
}manifestData;

manifestData* servermd = NULL;
manifestData* clientmd = NULL;
manifestData* livemd = NULL;

int serverVersion = -1;
int clientVersion = -1;

void func(int sockfd) 
{ 
    char buff[MAX]; 
    int n; 
    for (;;) { 
        bzero(buff, sizeof(buff)); 
        printf("Enter the string : "); 
        n = 0; 
        while ((buff[n++] = getchar()) != '\n') 
            ; 
        write(sockfd, buff, sizeof(buff)); 
        bzero(buff, sizeof(buff)); 
        read(sockfd, buff, sizeof(buff)); 
        printf("From Server : %s", buff); 
	break;
    } 
} 

int connectServer(){ //gets ip and port from file and connects
	int conf;
	char *ip_word;
	char *port_word;
	struct hostent *serverIP;
	struct sockaddr_in serverInfo;
	conf = open("configure", O_RDONLY);
	if (conf == -1){
		printf("config file does not exist\n");
		exit(1);
	}
	int currentPos = lseek(conf, 0, SEEK_CUR);
	int size = lseek(conf, 0, SEEK_END);	//get length of file
	lseek(conf, currentPos, SEEK_SET);  //set position back to start
	char c[size+1];
	int tracker = 0;
	if(read(conf, c, size) != 0){
		while (tracker < size){
			if (c[tracker] != '\n'){
				tracker++;
			}
			else{
				ip_word = (char*)malloc(sizeof(char)*(tracker+1));
				memcpy(ip_word, &c[0], tracker);
				ip_word[tracker] = '\0';
				printf("ip word is: %s\n", ip_word);
				port_word = (char*)malloc(sizeof(char)*(size-tracker-1));
				memcpy(port_word, &c[tracker+1], size-tracker-2);
				port_word[strlen(port_word)] = '\0';
				break;
			}
		}
	}
	close(conf);
	int port = atoi(port_word);	//convert string port to int
	serverIP = gethostbyname(ip_word);
	//printf("serverIP %s\n", serverIP->h_addr);
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1){
		printf("socket could not be created\n");
		exit(1);
	}
	//char myIP[16];
	serverInfo.sin_family = AF_INET;
	//bzero((char*)&serverInfo,sizeof(serverInfo));
	bcopy((char*)serverIP->h_addr, (char*)&serverInfo.sin_addr.s_addr, serverIP->h_length);
	//serverInfo.sin_addr.s_addr = inet_addr((char*)serverIP->h_addr); 
	serverInfo.sin_port = htons(port);
	//inet_ntop(AF_INET, &serverInfo.sin_addr, myIP, sizeof(myIP));
	//printf("Local ip address: %s\n", myIP);
	if (connect(sockfd,(struct sockaddr*)&serverInfo,sizeof(serverInfo)) != 0){
		printf("Connection to server failed.\n");
		exit(1);
	}
	else
		printf("Connection to server successful.....\n");
		return sockfd;
}

void configure(char* ip, char* port){ //create file with IP and port of server
		int conf;
		conf = creat("configure", O_APPEND | O_WRONLY | 0600);
		printf("Configuring.....\n");
		int len = strlen(ip) + strlen(port) + 3;
		char *ip_port = (char*) malloc(len);
		ip_port = strcpy(ip_port, ip);
		ip_port = strcat(ip_port, "\n"); //seperate IP and port with \n
		ip_port = strcat(ip_port, port);
		ip_port = strcat(ip_port, "\n");
		write(conf, ip_port, strlen(ip_port));
		printf("Configure successful.\n");
}

void create(char* name){
    char *str = (char*)malloc(sizeof(char)*(strlen(name)+9));
    str = strcpy(str, "create:");
    str = strcat(str, name);
    //printf("str is %s\n", str);
    int sockfd = connectServer();
    printf("Sending create command request to server.....\n");
    write(sockfd, str, strlen(str)+1);
    DIR* dir = opendir(name);
    if (ENOENT == errno){ //directory doesn't exist
    	mkdir(name, 0700);
    	char* path = (char*)malloc(sizeof(char)*(strlen(name)+12));
    	path = strcpy(path, name);
    	path = strcat(path, "/.Manifest");
    	int manifestFile;
    	manifestFile = creat(path, O_WRONLY | 0600);
    	if(manifestFile == -1){
	    printf("cannot create .Manifest\n");
   	}
    	write(manifestFile, "1", 1);
    	close(manifestFile);
    }
	char buff[MAX];
	//read(sockfd, buff, sizeof(buff));
	//printf("buff is %s\n", buff);
	close(sockfd);
}

char* createDigest(char* fileName, char* digest){
	int file = open(fileName, O_RDONLY);
	if (file == -1){
	    printf("File does not exist\n");
	    exit(1);
	}
	int currentPos = lseek(file, 0, SEEK_CUR);
	int size = lseek(file, 0, SEEK_END);    //get length of file
	lseek(file, currentPos, SEEK_SET);  //set position back to start
	char buffer[size+1];
	read(file, buffer, size);
	unsigned char hash[20];
	SHA1(buffer, sizeof(buffer)-1, hash);
	//digest[SHA_DIGEST_LENGTH*2];
	//digest = (char*)malloc(sizeof(char)*(SHA_DIGEST_LENGTH*2));
	int i;
	for (i=0; i < SHA_DIGEST_LENGTH; i++) {
    	sprintf((char*)&(digest[i*2]), "%02x", hash[i]);
    }
    //printf("SHA1 is %s\n", digest);
    close(file);
    return digest;
}

void checkAdd(char* fileName, char* dirName, char* digest){	//check if file is already in manifest and update if it is.
	char* manifestPath = (char*)malloc(sizeof(char)*(strlen(dirName)+12));
	manifestPath = strcpy(manifestPath, dirName);
	manifestPath = strcat(manifestPath, "/.Manifest");
	int manifest = open(manifestPath, O_RDWR);
	if (manifest == -1){
		printf("manifest doesn't exist\n");
		exit(1);
	}
	int currentPos = lseek(manifest, 0, SEEK_CUR);
	int size = lseek(manifest, 0, SEEK_END);    //get length of file
	lseek(manifest, currentPos, SEEK_SET);  //set position back to start
	char c[size+1];
	int tracker = 0;
	int linesize = 0;
	int isPresent = 0;
	if(read(manifest, c, size) != 0){
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
			if (strstr(line, fileName) != NULL){	//fileName is in this line in manifest
				isPresent = 1;
				int numPosition = tracker+2-linesize;
				int numSize = 0;
				while (c[numPosition] != ' '){
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
				//int ver = atoi(version);
				//ver++;
				//char strVer[ver+1];	//updated version number
				//sprintf(strVer, "%d", ver); //convert int to string
				lseek(manifest, currentPos, SEEK_SET); //set to start
				lseek(manifest, tracker-linesize, SEEK_SET);
				write(manifest, "U ", 2);
				write(manifest, version, strlen(version));
				write(manifest, " ", 1);
				write(manifest, fileName, strlen(fileName));
				write(manifest, " ", 1);
				write(manifest, digest, strlen(digest));
			}
			tracker++;
		}
		if (isPresent == 0){	//file not in manifest so add to the end
			lseek(manifest, currentPos, SEEK_SET); //set to start
			lseek(manifest, 0, SEEK_END); 
			write(manifest, "U 1 ", 4);
			write(manifest, fileName, strlen(fileName));
			write(manifest, " ", 1);
			write(manifest, digest, strlen(digest));
		}
	}
}

void add(char* dirName, char* fileName){
	DIR* dir = opendir(dirName);
	if (dir){	//directory exists
    }
	else if (ENOENT == errno){ //directory doesn't exist
		printf("%s doesn't exist\n", dirName);
		exit(1);
	}
	//char* filepath = (char*)malloc(sizeof(char)*(strlen(dirName)+strlen(fileName)+4));
	//filepath = strcpy(filepath, dirName);
	//filepath = strcat(filepath, "/");
	//filepath = strcat(filepath, fileName);
	if (access(fileName, F_OK) == -1){ // file doesn't exist
		printf("%s doesn't exist\n", fileName);
		exit(1);
	}
	char* digest = (char*)malloc(sizeof(char)*41);
	digest[41] = '\0';
	digest = createDigest(fileName, digest);
	//printf("digest %s\n", digest);
    char *writeStr = (char*)malloc(sizeof(char)*(sizeof(digest)+(strlen(fileName)+8)));
    strcpy(writeStr, "U 1 ");
    strcat(writeStr, fileName);
    strcat(writeStr, " ");
    strcat(writeStr, digest);
    //printf("string is %s\n", writeStr);
    checkAdd(fileName, dirName, digest);
    printf("updated manifest\n", fileName);
}

void removeFile(char* dirName, char* fileName){
	DIR* dir = opendir(dirName);
	if (dir){	//directory exists
    }
	else if (ENOENT == errno){ //directory doesn't exist
		printf("%s doesn't exist\n", dirName);
		exit(1);
	}
	//char* filepath = (char*)malloc(sizeof(char)*(strlen(dirName)+strlen(fileName)+4));
	//filepath = strcpy(filepath, dirName);
	//filepath = strcat(filepath, "/");
	//filepath = strcat(filepath, fileName);
	if (access(fileName, F_OK) == -1){ // file doesn't exist
		printf("%s doesn't exist\n", fileName);
		exit(1);
	}
	//remove file from manifest
	char* manifestPath = (char*)malloc(sizeof(char)*(strlen(dirName)+12));
	manifestPath = strcpy(manifestPath, dirName);
	manifestPath = strcat(manifestPath, "/.Manifest");
	int manifest = open(manifestPath, O_RDWR);
	if (manifest == -1){
		printf("manifest doesn't exist\n");
		exit(1);
	}
	int currentPos = lseek(manifest, 0, SEEK_CUR);
	int size = lseek(manifest, 0, SEEK_END);    //get length of file
	lseek(manifest, currentPos, SEEK_SET);  //set position back to start
	char c[size+1];
	int tracker = 0;
	int linesize = 0;
	char* newFilename = (char*)malloc(sizeof(char)*(strlen(dirName)+17));
	newFilename = strcpy(newFilename, dirName);
	newFilename = strcat(newFilename, "/tempfilerename");
	int newFile = creat(newFilename, O_APPEND | O_WRONLY | 0600);
	if(read(manifest, c, size) != 0){
		if (strstr(c, fileName) == NULL){
			printf("%s is not in the manifest\n", fileName);
			exit(1);
		}
		while (tracker < size){
			linesize = 0;
			while (c[tracker] != '\n'){
				tracker++;
				linesize++;
			}
			char* line = (char*)malloc(sizeof(char)*linesize+1);
			memcpy(line, &c[tracker-linesize], linesize);
			line[strlen(line)] = '\0';
			printf("line is %s\n", line);
			if (strstr(line, fileName) != NULL){	//skips line
				tracker++;
				continue;
			}
			write(newFile, line, strlen(line));
			write(newFile, "\n", 1);
			tracker++;
		}
	}
	close(manifest);
	close(newFile);
	remove(manifestPath);
	rename(newFilename, manifestPath);
}

void checkout(char* projectName){
	DIR* dir = opendir(projectName);
	if (dir){	//directory exists
		printf("project already exists\n");
		exit(1);
    	}
	int sockfd = connectServer();
	char *str = (char*)malloc(sizeof(char)*(strlen(projectName)+11));
	str = strcpy(str, "checkout:");
	str = strcat(str, projectName);
	printf("str is %s\n", str);
	printf("Sending checkout command request to server.....\n");
	write(sockfd, str, strlen(str));
	//read(sockfd, buff, 339);
	int len = 0;
	int totalSize = 0;
	while (!len && ioctl (sockfd,FIONREAD,&len) >= 0){
    	sleep(1);
    }
    char buff[len]; 
	if (len > 0) {
  		len = read(sockfd, buff, len);
  		totalSize += len;
	}
	//printf("totalSize is %i\n", totalSize);
	char* createTar = (char*)malloc(sizeof(char)*strlen(projectName)+5);
	strcpy(createTar, projectName);
	strcat(createTar, ".tgz");
	int newFile = creat(createTar, O_APPEND | O_RDWR | 0600);
	char* untar = (char*)malloc(sizeof(char)*strlen(createTar)+11);
	strcpy(untar, "tar -xvf ");
	strcat(untar, createTar);
	write(newFile, buff, len);
	system(untar);
	remove(createTar);
	//printf("From Server : %s", buff); 
	printf("completed checkout\n");
	
	close(sockfd);
}

void addManifestData(char* fileName, char* hash, int version, int clientOrServer){ //adds to manifestData struct. 0 is client 1 is server.
	//manifestData* mdPtr = mdclient;
	if (clientOrServer == 0){
		if (clientmd == NULL){
			clientmd = (manifestData*)malloc(sizeof(manifestData));
			clientmd->fileName = (char*)malloc(sizeof(char)*strlen(fileName)+1);
			strcpy(clientmd->fileName, fileName);
			clientmd->hash = (char*)malloc(sizeof(char)*41);
			strcpy(clientmd->hash, hash);
			clientmd->version = version;
			clientmd->next == NULL;
			//printf("client fileName %s\n", clientmd->fileName);
			//printf("client hash %s\n", clientmd->hash);
			//printf("client version %i\n", clientmd->version);
		}
		else{
			manifestData* mdptr = clientmd;
			while (mdptr->next != NULL){
				mdptr = mdptr->next;
			}
			manifestData* tmp = (manifestData*)malloc(sizeof(manifestData));
			tmp->fileName = (char*)malloc(sizeof(char)*strlen(fileName)+1);
			strcpy(tmp->fileName, fileName);
			tmp ->hash = (char*)malloc(sizeof(char)*41);
			strcpy(tmp->hash, hash);
			tmp->version = version;
			tmp->next = NULL;
			mdptr->next = tmp;
		}
	}
	else if (clientOrServer == 1){
		if (servermd == NULL){
			servermd = (manifestData*)malloc(sizeof(manifestData));
			servermd->fileName = (char*)malloc(sizeof(char)*strlen(fileName)+1);
			strcpy(servermd->fileName, fileName);
			servermd->hash = (char*)malloc(sizeof(char)*41);
			strcpy(servermd->hash, hash);
			servermd->version = version;
			servermd->next = NULL;
			//printf("server fileName %s\n", servermd->fileName);
			//printf("server hash %s\n", servermd->hash);
			//printf("server version %i\n", servermd->version);
		}
		else{
			manifestData* mdptr = servermd;
			while (mdptr->next != NULL){
				mdptr = mdptr->next;
			}
			manifestData* tmp = (manifestData*)malloc(sizeof(manifestData));
			tmp->fileName = (char*)malloc(sizeof(char)*strlen(fileName)+1);
			strcpy(tmp->fileName, fileName);
			tmp ->hash = (char*)malloc(sizeof(char)*41);
			strcpy(tmp->hash, hash);
			tmp->version = version;
			tmp->next = NULL;
			mdptr->next = tmp;
			//printf("s fileName %s\n", mdptr->next->fileName);
			//printf("s hash %s\n", mdptr->next->hash);
			//printf("s version %i\n", mdptr->next->version);
		}
	}
}

void addLiveManifestData(char* fileName, char* hash){
	manifestData* mdliveptr = livemd;
	if (livemd == NULL){
		livemd = (manifestData*)malloc(sizeof(manifestData));
		livemd->fileName = (char*)malloc(sizeof(char)*strlen(fileName)+1);
		strcpy(livemd->fileName, fileName);
		livemd->hash = (char*)malloc(sizeof(char)*41);
		strcpy(livemd->hash, hash);
		livemd->next = NULL;
		//printf("1st livemd fileName %s\n", livemd->fileName);
		//printf("1st livemd hash %s\n", livemd->hash);
	}
	else{
		while (mdliveptr->next != NULL){
			mdliveptr = mdliveptr->next;
		}
		manifestData* tmp = (manifestData*)malloc(sizeof(manifestData));
		tmp->fileName = (char*)malloc(sizeof(char)*strlen(fileName)+1);
		strcpy(tmp->fileName, fileName);
		tmp->hash = (char*)malloc(sizeof(char)*41);
		strcpy(tmp->hash, hash);
		tmp->next = NULL;
		mdliveptr->next = tmp;
		//printf("tmp fileName %s\n", mdliveptr->next->fileName);
		//printf("tmp hash %s\n", mdliveptr->next->hash);
	}
}

void getLiveManifestData(char* pathorfile){	//goes through all subdirectories and finds files and gets the hashes for the files.
   //printf("Current path: %s\n", pathorfile);
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
       	//printf("Directory: %s\n", new);
       	getLiveManifestData(new);
   	}
   	else if(S_ISREG(path_stat.st_mode)){ //is a file
       	//printf("File: %s\n", new);
       	char* digest = (char*)malloc(sizeof(char)*41);
       	digest[41] = '\0';
       	createDigest(new, digest);
       	addLiveManifestData(new, digest);
       	free(digest);
       	}
   	
   }
   closedir(pDir);
}

void getClientManifestData(char* projectName){
	char* manifestPath = (char*)malloc(sizeof(char)*(strlen(projectName)+12));
	manifestPath = strcpy(manifestPath, projectName);
	manifestPath = strcat(manifestPath, "/.Manifest");
	int manifestFile = open(manifestPath, O_RDWR);
	if (manifestFile == -1){
		printf("client manifest doesn't exist\n");
		exit(1);
	}
	int currentPos = lseek(manifestFile, 0, SEEK_CUR);
	int size = lseek(manifestFile, 0, SEEK_END);    //get length of file
	lseek(manifestFile, currentPos, SEEK_SET);  //set position back to start
	char c[size+1];
	int tracker = 0;
	int linesize = 0;
	int firstline = 0;
	if(read(manifestFile, c, size) != 0){
		while (tracker < size){
			linesize = 0;
			while (c[tracker] != '\n'){
				tracker++;
				linesize++;
			}
			if (linesize <2){ //skips first line
				if (firstline == 0){
					char* totalVersion = (char*)malloc(sizeof(char)*linesize+1);
					memcpy(totalVersion, &c[tracker-linesize], linesize);
					totalVersion[linesize] = '\0';
					clientVersion = atoi(totalVersion);
					//printf("totalVersion is %i\n", clientVersion);
					firstline++;
				}
				tracker++;
				continue;	//line doces not have contain a filename
			}
			char* line = (char*)malloc(sizeof(char)*linesize+1);
			memcpy(line, &c[tracker-linesize], linesize);
			line[linesize] = '\0';
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
			memcpy(version, &c[tracker+2-linesize], numSize);
			version[numSize] = '\0';
			int ver = atoi(version);
			int fileNameSize = linesize-44-strlen(version)+1; //with null teminator
			//printf("linesize is %i\n", linesize);
			//printf("fileNamesize is %i\n", fileNameSize);
			char* fileName = (char*)malloc(sizeof(char)*fileNameSize);
			memcpy(fileName, &c[(tracker-linesize)+3+strlen(version)], fileNameSize-1);
			fileName[fileNameSize-1] = '\0';
			char* hash = (char*)malloc(sizeof(char)*41);
			memcpy(hash, &c[tracker-linesize+strlen(version)+strlen(fileName)+4], 40);
			hash[41] = '\0';
			//printf("fileName is %s\n", fileName);
			//printf("hash is %s\n", hash);
			addManifestData(fileName, hash, ver, 0);
			free(fileName);
			free(hash);
		}
	}
	close(manifestFile);
}

void getServerManifestData(char* projectName){
	int sockfd = connectServer();
	char* message = (char*)malloc(sizeof(char)*strlen(projectName+9));
	strcpy(message, "update:");
	strcat(message, projectName);
	printf("message is %s\n", message);
	write(sockfd, message, strlen(message)+1);
	int len = 0;
	int size = 0;
	while (!len && ioctl (sockfd,FIONREAD,&len) >= 0){
    	sleep(1);
    }
    char buff[len+1]; 
	if (len > 0) {
  		len = read(sockfd, buff, len);
  		size += len;
	}
	//printf("size is %i\n", size);
	if (strcmp(buff, "exit") == 0){
		printf("Error retrieving .Manifest from server\n");
		exit(1);
	}
	int tracker = 0;
	int linesize = 0;
	int firstline = 0;
	while (tracker < size){
		linesize = 0;
		while (buff[tracker] != '\n' && buff[tracker] != '\0'){
			tracker++;
			linesize++;
		}
		if (linesize <2){ //skips first line
			if (firstline == 0){	//gets version of manifest on first line
				char* totalVersion = (char*)malloc(sizeof(char)*linesize+1);
				memcpy(totalVersion, &buff[tracker-linesize], linesize);
				totalVersion[linesize] = '\0';
				serverVersion = atoi(totalVersion);
				//printf("totalVersion is %i\n", clientVersion);
				firstline++;
			}
			tracker++;
			continue;	//line doces not have contain a filename
		}
		char* line = (char*)malloc(sizeof(char)*linesize+1);
		memcpy(line, &buff[tracker-linesize], linesize);
		line[linesize] = '\0';
		//printf("line is %s\n", line);
		int numPosition = tracker+2-linesize;
		int numSize = 0;
		while (buff[numPosition] != ' '){	//gets length of version
			numPosition++;
			numSize++;
			if (numSize > 500){
				printf("error\n");
				exit(1);
			}
		}
		char* version = (char*)malloc(sizeof(char)*(numSize+1));
		memcpy(version, &buff[tracker+2-linesize], numSize);
		version[numSize] = '\0';
		int ver = atoi(version);
		int fileNameSize = linesize-44-strlen(version)+1; //with null teminator
		//printf("linesize is %i\n", linesize);
		//printf("fileNamesize is %i\n", fileNameSize);
		char* fileName = (char*)malloc(sizeof(char)*fileNameSize);
		memcpy(fileName, &buff[(tracker-linesize)+3+strlen(version)], fileNameSize-1);
		fileName[fileNameSize-1] = '\0';
		char* hash = (char*)malloc(sizeof(char)*41);
		memcpy(hash, &buff[tracker-linesize+strlen(version)+strlen(fileName)+4], 40);
		hash[41] = '\0';
		//printf("version is %s\n", version);
		//printf("fileName is %s\n", fileName);
		//printf("hash is %s\n", hash);
		addManifestData(fileName, hash, ver, 1);
		free(fileName);
		free(hash);
		free(version);
	}
	
}

void printManifestData(){
	manifestData* mdClientPtr = clientmd;
	manifestData* mdServerPtr = servermd;
	manifestData* mdLivePtr = livemd;
	
	printf("client manifest data and version is %i\n", clientVersion);
	while (mdClientPtr != NULL){
		printf("fileName is %s\n", mdClientPtr->fileName);
		printf("hash is %s\n", mdClientPtr->hash);
		printf("version is %i\n", mdClientPtr->version);
		mdClientPtr = mdClientPtr->next;
	}
	printf("\n");
	printf("server manifest data and version is %i\n", serverVersion);
	while (mdServerPtr != NULL){
		printf("fileName is %s\n", mdServerPtr ->fileName);
		printf("hash is %s\n", mdServerPtr ->hash);
		printf("version is %i\n", mdServerPtr ->version);
		mdServerPtr = mdServerPtr ->next;
	}
	printf("\n");
	printf("live manifest data is\n");
	while (mdLivePtr != NULL){
		printf("fileName is %s\n", mdLivePtr->fileName);
		printf("hash is %s\n", mdLivePtr->hash);
		mdLivePtr = mdLivePtr->next;
	}
	printf("\n");
}
void upload(){	
	manifestData* mdClientPtr = clientmd;
	manifestData* mdServerPtr = servermd;
	manifestData* mdLivePtr = livemd;
	while(mdClientPtr !=NULL){	//file in clients manifest but not servers
		mdServerPtr = servermd;
		while (mdServerPtr != NULL){
			if (strcmp(mdClientPtr->fileName, mdServerPtr->fileName) == 0){
				break;
			}
			mdServerPtr = mdServerPtr->next;
		}
		if (mdServerPtr == NULL){
			printf("U %s\n", mdClientPtr->fileName);
		}
		mdClientPtr = mdClientPtr->next;
	}
	
	mdClientPtr = clientmd;
	mdServerPtr = servermd;
	mdLivePtr = livemd;
	while(mdLivePtr != NULL){	//server and client both have but and live hash are different
		mdServerPtr = servermd;
		//printf("server fileName1 is %s\n", mdServerPtr->fileName);
		while(mdServerPtr != NULL){
			if (strcmp(mdLivePtr->fileName, mdServerPtr->fileName) == 0){
				if (strcmp(mdLivePtr->hash, mdServerPtr->hash) != 0){
					printf("U %s\n", mdLivePtr->fileName);
				}
			}
			mdServerPtr = mdServerPtr->next;
		}
		mdLivePtr = mdLivePtr->next;
	}
	
}
void modify(){	//file both server and client have, but file version different and the clients live hash is same as its manifest

}
void update(char* projectName){
	getServerManifestData(projectName);
	getClientManifestData(projectName);
	getLiveManifestData(projectName);
	printManifestData();
	
	manifestData* mdClientPtr = clientmd;
	manifestData* mdServerPtr = servermd;
	manifestData* mdLivePtr = livemd;
	
	if (clientVersion == -1){
		printf("could not get client manifest version\n");
		exit(1);
	}
	if (serverVersion == -1){
		printf("could not get server manifest version\n");
		exit(1);
	}
	if (clientVersion == serverVersion){
		upload();
	}
	else{
		modify();
	}
}

int main(int argc, char *argv[]){
	if (argc < 2 || argc > 4){
		printf("Incorrect number of arguments");
		exit(1);
	}
	if (strcmp(argv[1],"configure") == 0){
		if (argc != 4){
			printf("Incorrect number of arguments for configure");
			exit(1);
		}
		configure(argv[2], argv[3]);
		//int sockfd = connectServer();
		//func(sockfd);
		//close(sockfd);
	}
	else if (strcmp(argv[1], "create")== 0){
		if (argc !=3){
			printf("Incorrect number of arguments for create\n");
			exit(1);
		}
		create(argv[2]);
		//printf("create\n");
	}
	else if (strcmp(argv[1], "add") == 0){
		if (argc !=4){
			printf("Incorrect number of arguments for add\n");
			exit(1);
		}
		add(argv[2], argv[3]);
	}
	else if (strcmp(argv[1], "remove") == 0){
		if (argc !=4){
			printf("Incorrect number of arguments for remove\n");
			exit(1);
		}
		removeFile(argv[2],argv[3]);
	}
	else if (strcmp(argv[1], "checkout") == 0){
		if (argc !=3){
			printf("Incorrect number of arguments for checkout\n");
			exit(1);
		}
		checkout(argv[2]);
	}
	else if (strcmp(argv[1], "update") == 0){
		if (argc != 3){
			printf("Incorrect number of arguments for update\n");
			exit(1);
		}
		update(argv[2]);
	printf("Client command completed.\n");
	}
}
