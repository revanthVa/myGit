#include "WTF.h"

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
				//printf("ip word is: %s\n", ip_word);
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
	//check if port number is valid
	//printf("port is %s\n", port);
	int i;
    for(i = 0; i<strlen(port); i++){
	//printf("current char: %c\n", port[i]);
		if(!isdigit(port[i])){
			printf("Invalid port number\n");
			exit(1);
		}
	}
    int num = atoi(port);
    //printf("num is %i\n", num);
	if (num < 8000 || num > 65535){
		printf("Please enter a valid port number between 8000 and 65535\n");
		exit(1);
	}
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
    
    int len = 0;
    int timeouts = 0;
    while (!len && ioctl(sockfd,FIONREAD,&len) >= 0){
		sleep(1);
		timeouts++;
		if (timeouts == 5){
			printf("Error. Server timed out\n");
			close(sockfd);
			exit(1);
		}
	  }
	  char buff[len+1]; 
	  if (len > 0) {
	      len = read(sockfd, buff, len);
	  }
	  //printf("size is %i\n", size);
	  if (strcmp(buff, "exit") == 0){
	    printf("Create failed. Directory may already exist on server\n");
	    exit(1);
	  }
    
    DIR* dir = opendir(name);
    char* path = (char*)malloc(sizeof(char)*(strlen(name)+12));
    path = strcpy(path, name);
    path = strcat(path, "/.Manifest");
    if (ENOENT == errno){ //directory doesn't exist
    	mkdir(name, 0700);
    	int manifestFile;
    	manifestFile = creat(path, O_WRONLY | 0600);
    	if(manifestFile == -1){
	   		printf("cannot create .Manifest\n");
   		}
    	write(manifestFile, "1\n", 2);
    	close(manifestFile);
    }
    else if (dir){ //checks if local copy has a manifestFile
      printf("directory already exists on client\n");
      int file = open(path, O_RDONLY);
      if (file == -1){
	  printf("The project was created successfully in the server repository but creating it in local repository failed because a directory with that name already exists. Please remove/rename and use checkout to obtain a copy of the newly created project.\n");
	  exit(1);
      }
      close(file);
    }
    //char buff[MAX];
    //read(sockfd, buff, sizeof(buff));
    //printf("buff is %s\n", buff);
    close(sockfd);
}

void destroy(char* name){
    char *str = (char*)malloc(sizeof(char)*(strlen(name)+10));
    str = strcpy(str, "destroy:");
    str = strcat(str, name);
    //printf("str is %s\n", str);
    int sockfd = connectServer();
    printf("Sending destroy command request to server.....\n");
    write(sockfd, str, strlen(str)+1);
    DIR* dir = opendir(name);
    //then delete the manifest file on client if directory exists.
    if (dir){ //directory exists
	char* manifestPath = (char*)malloc(sizeof(char)*(strlen(name)+12));
	manifestPath = strcpy(manifestPath, name);
	manifestPath = strcat(manifestPath, "/.Manifest");
	//printf("manifest path is: %s\n", manifestPath);
	remove(manifestPath);
    }
    else if (ENOENT == errno)	//directory doesn't exist
    {
	printf("Client side does not have local copy of project.\n");
    }
    else{
    	printf("Error destroying project directory on client side.\n");
    }
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
			if (linesize < 10){ //skips first line
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
				write(manifest, "  ", 2);
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
			write(manifest, "  1 ", 4);
			write(manifest, fileName, strlen(fileName));
			write(manifest, " ", 1);
			write(manifest, digest, strlen(digest));
			write(manifest, "\n", 1);
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
	strcpy(writeStr, "  1 ");
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
	//if (access(fileName, F_OK) == -1){ // file doesn't exist
		//printf("%s doesn't exist\n", fileName);
		//exit(1);
	//}
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
			//printf("line is %s\n", line);
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
	//printf("str is %s\n", str);
	printf("Sending checkout command request to server.....\n");
	write(sockfd, str, strlen(str));
	//read(sockfd, buff, 339);
	int len = 0;
	int totalSize = 0;
	int timeouts = 0;
	while (!len && ioctl (sockfd,FIONREAD,&len) >= 0){
	  sleep(1);
	  timeouts++;
	  if (timeouts == 5){
	  	printf("Error no response from server");
	  }
	}
	char buff[len]; 
	if (len > 0) {
  		len = read(sockfd, buff, len);
  		totalSize += len;
	}
	if (strcmp(buff, "exit") == 0){
		printf("Error. Server timed out while creating the commit.\n");
		exit(1);
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
	    manifestData* mdClientPtr = clientmd;
	    while (mdClientPtr != NULL && strcmp(mdClientPtr->fileName, new) != 0){
			    mdClientPtr = mdClientPtr->next;
	    }
	    if (mdClientPtr == NULL){
		    continue;
	    }
	    else if (strcmp(mdClientPtr->fileName, new) == 0){
		    //printf("yes they are the same %s\n", new);
			    char* digest = (char*)malloc(sizeof(char)*41);
			    digest[41] = '\0';
			    createDigest(new, digest);
			    addLiveManifestData(new, digest);
			    free(digest);
	    }
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
			if (linesize < 10){ //skips first line
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
    //printf("message is %s\n", message);
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
		if (linesize < 10){ //skips first line
			if (firstline == 0){	//gets version of manifest on first line
				char* totalVersion = (char*)malloc(sizeof(char)*linesize+1);
				memcpy(totalVersion, &buff[tracker-linesize], linesize);
				totalVersion[linesize] = '\0';
				serverVersion = atoi(totalVersion);
				//printf("totalVersion is %i\n", clientVersion);
				firstline++;
			}
			tracker++;
			continue;	//line does not have contain a filename
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
		close(sockfd);
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
void updateUpload(int updateFile){	
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
			char* writeStr = (char*)malloc(sizeof(char)*strlen(mdClientPtr->fileName)+5);
			strcpy(writeStr, "U ");
			strcat(writeStr, mdClientPtr->fileName);
			strcat(writeStr, "\n");
			//write(updateFile, writeStr, strlen(writeStr));
			free(writeStr);
		}
		mdClientPtr = mdClientPtr->next;
	}
	
	mdClientPtr = clientmd;
	mdServerPtr = servermd;
	mdLivePtr = livemd;
	while(mdLivePtr != NULL){	//server and client both have but server and live hash are different
		mdServerPtr = servermd;
		//printf("server fileName1 is %s\n", mdServerPtr->fileName);
		while(mdServerPtr != NULL){
			if (strcmp(mdLivePtr->fileName, mdServerPtr->fileName) == 0){
				if (strcmp(mdLivePtr->hash, mdServerPtr->hash) != 0){
					printf("U %s\n", mdLivePtr->fileName);
					char* writeStr = (char*)malloc(sizeof(char)*strlen(mdLivePtr->fileName)+5);
					strcpy(writeStr, "U ");
					strcat(writeStr, mdLivePtr->fileName);
					strcat(writeStr, "\n");
					//write(updateFile, writeStr, strlen(writeStr));
					free(writeStr);
					break;
				}
			}
			mdServerPtr = mdServerPtr->next;
		}
		mdLivePtr = mdLivePtr->next;
	}
	
}

void updateModify(int updateFile){	//file both server and client have, but file version different and the clients live hash is same as its manifest
// or file both server and client have, but file version same and hash different
	manifestData* mdClientPtr = clientmd;
	manifestData* mdServerPtr = servermd;
	manifestData* mdLivePtr = livemd;
	
	while (mdClientPtr != NULL){
		mdServerPtr = servermd;
		while (mdServerPtr !=NULL){
			if (strcmp(mdClientPtr->fileName, mdServerPtr->fileName) == 0){
				if (mdClientPtr->version != mdServerPtr->version){
					//printf("file version different \n");
					mdLivePtr = livemd;
					while(mdLivePtr != NULL && (strcmp(mdLivePtr->fileName, mdClientPtr->fileName) != 0)){
						mdLivePtr = mdLivePtr->next;
					}
					if (mdLivePtr == NULL){
						break;
					}
					if (strcmp(mdLivePtr->fileName, mdClientPtr->fileName) == 0){
						if (strcmp(mdLivePtr->hash, mdClientPtr->hash) == 0){
							printf("M %s\n", mdClientPtr->fileName);
							char* writeStr = (char*)malloc(sizeof(char)*strlen(mdClientPtr->fileName)+5);
							strcpy(writeStr, "M ");
							strcat(writeStr, mdClientPtr->fileName);
							strcat(writeStr, "\n");
							write(updateFile, writeStr, strlen(writeStr));
							free(writeStr);
						}
					}
				}
				else if (mdClientPtr->version == mdServerPtr->version){
					mdLivePtr = livemd;
					while(mdLivePtr != NULL && (strcmp(mdLivePtr->fileName, mdClientPtr->fileName) != 0)){
						mdLivePtr = mdLivePtr->next;
					}
					if (mdLivePtr == NULL){
						break;
					}
					if (strcmp(mdLivePtr->fileName, mdClientPtr->fileName) == 0){
						if (strcmp(mdLivePtr->hash, mdClientPtr->hash) != 0){
							printf("M %s\n", mdClientPtr->fileName);
							char* writeStr = (char*)malloc(sizeof(char)*strlen(mdClientPtr->fileName)+5);
							strcpy(writeStr, "M ");
							strcat(writeStr, mdClientPtr->fileName);
							strcat(writeStr, "\n");
							write(updateFile, writeStr, strlen(writeStr));
							free(writeStr);
						}
					}
				}
			}
			mdServerPtr = mdServerPtr->next;
		}
		mdClientPtr = mdClientPtr->next;
	}
}

void updateAdd(int updateFile){	//file in servers manifest but not in clients
	manifestData* mdClientPtr = clientmd;
	manifestData* mdServerPtr = servermd;
	manifestData* mdLivePtr = livemd;
	
	while(mdServerPtr != NULL){	
		mdClientPtr = clientmd;
		while (mdClientPtr != NULL){
			if (strcmp(mdClientPtr->fileName, mdServerPtr->fileName) == 0){
				break;
			}
			mdClientPtr = mdClientPtr->next;
		}
		if (mdClientPtr == NULL){
			printf("A %s\n", mdServerPtr->fileName);
			char* writeStr = (char*)malloc(sizeof(char)*strlen(mdServerPtr->fileName)+5);
			strcpy(writeStr, "A ");
			strcat(writeStr, mdServerPtr->fileName);
			strcat(writeStr, "\n");
			write(updateFile, writeStr, strlen(writeStr));
			free(writeStr);
		}
		mdServerPtr = mdServerPtr->next;
	}
}

void updateDelete(int updateFile){
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
			printf("D %s\n", mdClientPtr->fileName);
			char* writeStr = (char*)malloc(sizeof(char)*strlen(mdClientPtr->fileName)+5);
			strcpy(writeStr, "D ");
			strcat(writeStr, mdClientPtr->fileName);
			strcat(writeStr, "\n");
			write(updateFile, writeStr, strlen(writeStr));
			free(writeStr);
		}
		mdClientPtr = mdClientPtr->next;
	}
}

int updateConflicts(){	//manifest different, file version different and client live hash different
	manifestData* mdClientPtr = clientmd;
	manifestData* mdServerPtr = servermd;
	manifestData* mdLivePtr = livemd;
	int conflictPresent = 0;
	while (mdClientPtr != NULL){
		mdServerPtr = servermd;
		while (mdServerPtr !=NULL){
			if (strcmp(mdClientPtr->fileName, mdServerPtr->fileName) == 0){
				if (mdClientPtr->version != mdServerPtr->version){
					//printf("file version different \n");
					mdLivePtr = livemd;
					while(mdLivePtr != NULL && (strcmp(mdLivePtr->fileName, mdClientPtr->fileName) != 0)){
						mdLivePtr = mdLivePtr->next;
					}
					if (mdLivePtr == NULL){
						break;
					}
					if (strcmp(mdLivePtr->fileName, mdClientPtr->fileName) == 0){
						if (strcmp(mdLivePtr->hash, mdClientPtr->hash) != 0){
							printf("Conflict %s\n", mdClientPtr->fileName);
							conflictPresent = 1;
						}
					}
				}
			}
			mdServerPtr = mdServerPtr->next;
		}
		mdClientPtr = mdClientPtr->next;
	}
	return conflictPresent;
}

void update(char* projectName){
	getClientManifestData(projectName);
	getLiveManifestData(projectName);
	getServerManifestData(projectName);
	//printManifestData();
	
	manifestData* mdClientPtr = clientmd;
	manifestData* mdServerPtr = servermd;
	manifestData* mdLivePtr = livemd;
	
	if (clientVersion == -1){
		printf("Could not get client manifest version\n.");
		exit(1);
	}
	if (serverVersion == -1){
		printf("Could not get server manifest version\n.");
		exit(1);
	}
	char* path = (char*)malloc(sizeof(char)*(strlen(projectName)+10));
   	path = strcpy(path, projectName);
    path = strcat(path, "/.Update");
	int updateFile = creat(path, O_APPEND | O_RDWR | 0600);
	if (clientVersion != serverVersion){
		int conflictPresent = updateConflicts();
		if (conflictPresent == 1){
			printf("Please resolve the conflicts before updating\n");
			exit(1);
		}
	}
	
	if (clientVersion == serverVersion){
		updateUpload(updateFile);
	}
	else{
		updateModify(updateFile);
		updateAdd(updateFile);
		updateDelete(updateFile);
	}
	int size = lseek(updateFile, 0, SEEK_END);
	if(size == 0){
		printf("Up to date\n");
	}
	close(updateFile);
}

void commit(char* projectName){
      DIR* dir = opendir(projectName);
      if (dir){ //directory exists
	  char* updatePath = (char*)malloc(sizeof(char)*(strlen(projectName)+9));
	  updatePath = strcpy(updatePath, projectName);
	  updatePath = strcat(updatePath, "/.Update");
	  int file = open(updatePath, O_RDONLY);
	  if(file == -1){ //update file doesn't exist
	  }
	  else{ //if update file exists, check if empty
	      int size = lseek(file, 0, SEEK_END); //get length of file
	      if(size != 0){
		  printf("There are pending updates need to be made. Upgrade first and commit again.\n");
		  exit(0);
	      }
	  }
	  close(file);
	  free(updatePath);
      }
      else if (ENOENT == errno){    //directory doesn't exist
	  printf("Client side does not have local copy of project.\n");
	  exit(1);
      }
      else{
	  printf("Error accessing directory.\n");
      }
      printf("getting client manifest data\n");
      getClientManifestData(projectName);
      printf("getting server manifest data\n");
      getServerManifestData(projectName);
      printf("getting live manifest data\n");
      getLiveManifestData(projectName);

      if (clientVersion != serverVersion){
	  printf("Client version is not up to date with server version. Please update the local project first.\n");
	  exit(1);
      }
      else{ //version numbers match
	  //compute hashes
	  char* path = (char*)malloc(sizeof(char)*(strlen(projectName)+10));
	  path = strcpy(path, projectName);
	  path = strcat(path, "/.Commit");
	  //printf("path of commit: %s\n", path);
	  int commitFile = creat(path, O_APPEND | O_RDWR | 0600);
	  if(commitFile == -1){
	      printf("cannot create .Commit\n");
	      exit(1);
	  }
	  manifestData* liveptr = livemd;
	  manifestData* serverptr = servermd;
	  manifestData* clientptr = clientmd;
	  //printManifestData();
	  /*- get the server's .Manifest and compare all entries in it with the client's .Manifest and find out
	  which files the client has that are newer versions than the ones on the server, or the server does
	  not have, and write out a .Commit recording all the changes that need to be made.*/
	  while(clientptr != NULL){
	      serverptr = servermd;
	      if(serverptr == NULL){
		printf("serverptr already null\n");
	      }
	      liveptr = livemd;
	      if(liveptr == NULL){
		printf("liveptr already null\n");
	      }
	      if(serverptr != NULL){
		    while(strcmp(clientptr->fileName, serverptr->fileName) != 0){
			  serverptr = serverptr->next;
			  if(serverptr == NULL){
			      break;
			  }
		    }
	      }
	      if(liveptr != NULL){
		    while(strcmp(clientptr->fileName, liveptr->fileName) != 0){ //find file inside of livemd, get version number and increment it
			  liveptr = liveptr->next;
			  if(liveptr == NULL){
				break;
			  }
		    }
	      }
	      if(serverptr == NULL){ //clientfilename is not inside of server, but in the client
		    //indicate that file needs to be added to the repository
		    int version = clientptr->version;
		    if(liveptr != NULL){
			  if(strcmp(liveptr->hash, clientptr->hash) != 0){
			      version = version + 1;
			  }
		    }
		    int temp = version;
		    int count = 0;
		    while(temp != 0){
			  temp = temp/10;
			  ++count;
		    }
		    char strversion[count];
		    sprintf(strversion, "%i", version);
		    //int writeStrlen = strlen
		    char* writeStr = (char*)malloc(sizeof(char)*(strlen(clientptr->fileName)+strlen(clientptr->hash)+strlen(strversion)+12));
		    strcpy(writeStr, "A ");
		    strcat(writeStr, strversion);
		    strcat(writeStr, " ");
		    strcat(writeStr, clientptr->fileName);
		    strcat(writeStr, " ");
		    if(liveptr == NULL){ //hash not computed for client manifest file since its deleted in the current client's directory
			  strcat(writeStr, clientptr->hash);
		    }
		    else{
			  strcat(writeStr, liveptr->hash);
		    }
		    strcat(writeStr, "\n");
		    write(commitFile, writeStr, strlen(writeStr));
		    free(writeStr);
	      }
	      else { //compare hashes between live and server, since server file is found
		    if(liveptr == NULL){ //located client manifest file is also inside of server, but not inside livemd.
			  clientptr = clientptr->next;
			  continue;
		    }
		    //version number of client file should be incremented (inside of .Commit)
		    //indicate that file needs to be updated
		    if(strcmp(liveptr->hash, serverptr->hash) != 0){
			  int newversion = clientptr->version + 1;
			  int temp = newversion;
			  int count = 0; //how many digits the version value is
			  while(temp != 0){
			      temp = temp/10;
			      ++count;
			  }
			  if(newversion <= serverptr->version){ //report commit failed
			      printf("Client needs to synch with update+upgrade before commiting. Commit failed.\n");
			      close(commitFile);
			      remove(path);
			      exit(1);
			  }
			  char strnewversion[count];
			  sprintf(strnewversion, "%i", newversion); //convert int to string
			  char* writeStr = (char*)malloc(sizeof(char)*(strlen(liveptr->fileName)+strlen(liveptr->hash)+strlen(strnewversion)+10));
			  strcpy(writeStr, "U ");
			  strcat(writeStr, strnewversion);
			  strcat(writeStr, " ");
			  strcat(writeStr, liveptr->fileName);
			  strcat(writeStr, " ");
			  strcat(writeStr, liveptr->hash);
			  strcat(writeStr, "\n");
			  write(commitFile, writeStr, strlen(writeStr));
			  free(writeStr);
		    }
		    //else, filename exists in both client and server and hash values are the same, so do nothing
	      }
	      clientptr = clientptr->next;
	  }
	  serverptr = servermd;
	  clientptr = clientmd;
	  liveptr = livemd;
	  while(serverptr != NULL){ //search if any server filenames do not exist inside client
	      clientptr = clientmd;
	      liveptr = livemd;
	      if(clientptr != NULL){
		    while(strcmp(clientptr->fileName, serverptr->fileName) != 0){
			  clientptr = clientptr->next;
			  if(clientptr == NULL){
			      break;
			  }
		    }
	      }
	      if(liveptr != NULL){
		    if(clientptr != NULL){
			while(strcmp(liveptr->fileName, clientptr->fileName) != 0){
			      liveptr = liveptr->next;
			      if(liveptr == NULL){
				    break;
			      }
			}
		    }
	      }
	      if(clientptr == NULL){ //current server file does not exist inside of client
		    //indicate that file needs to be deleted to the repository
		    int version = serverptr->version;
		    int temp = version;
		    int count = 0; //how many digits the version value is
		    while(temp != 0){
		      temp = temp/10;
		      ++count;
		    }
		    char strversion[count];
		    sprintf(strversion, "%i", version); //convert int to string
		    char* writeStr = (char*)malloc(sizeof(char)*(strlen(liveptr->fileName)+strlen(liveptr->hash)+strlen(strversion)+10));
		    strcpy(writeStr, "D ");
		    strcat(writeStr, strversion);
		    strcat(writeStr, " ");
		    strcat(writeStr, serverptr->fileName);
		    strcat(writeStr, " ");
		    if(liveptr == NULL){ //hash not computed for client manifest file since its deleted in the current client's directory
			  strcat(writeStr, serverptr->hash);
		    }
		    else{
			  strcat(writeStr, liveptr->hash);
		    }
		    strcat(writeStr, "\n");
		    write(commitFile, writeStr, strlen(writeStr));
		    free(writeStr);
	      }
	      serverptr = serverptr->next;
	  }
	  int commitSize = lseek(commitFile, 0, SEEK_END); //get length of file
	  if(commitSize == 0){
	    printf("No changes detected nor differences between server and manifests found. Deleting empty commit.\n");
	    close(commitFile);
	    remove(path);
	    exit(0);
	  }
	  close(commitFile);
	  commitFile = open(path, O_RDONLY);
	  //report success
	  printf("Commit successfully created. Sending commit command request to server.....\n");
	  int sockfd = connectServer();
	  //path = "projectName/.Commits/name";
	  int currentPos = lseek(commitFile, 0, SEEK_CUR);
	  int size = lseek(commitFile, 0, SEEK_END);    //get length of file
	  lseek(commitFile, currentPos, SEEK_SET);  //set position back to start
	  char c[size+1];
	  //printf("size is %i\n", size);
	  char* sendStr = (char*)malloc(sizeof(char)*(strlen(projectName)+size+13));
	  strcpy(sendStr, "commit:");
	  strcat(sendStr, projectName);
	  /*strcat(sendStr, ":");
	  strcat(sendStr, c);*/
	  //size = size + 1;
	  write(sockfd, sendStr, strlen(sendStr));
	  int len = 0;
	  int timeouts = 0;
	  sleep(1);
	  if(read(commitFile, c, size) != 0){
	    printf("Reading commit file:\n");
	    c[size+1] = '\0';
	    printf("%s\n", c);
	    write(sockfd, c, size);
	  }
	  printf("Waiting on server.......\n");
	  while (!len && ioctl(sockfd,FIONREAD,&len) >= 0){
		sleep(1);
		timeouts++;
		if (timeouts == 5){
			printf("No error reported by the server. Commit successfully sent to server.\n");
			close(sockfd);
			exit(1);
		}
	  }
	  printf("finished sleeps\n");
	  char buff[len+1]; 
	  if (len > 0) {
	      len = read(sockfd, buff, len);
	  }
	  //printf("size is %i\n", size);
	  if (strcmp(buff, "exit") == 0){
	    printf("Error. Server timed out while creating the commit.\n");
	    remove(path);
	    exit(1);
	  }
 	  //close(commitFile);
      }
}
     

void currentversion(char* projectName){
	char* sendStr = (char*)malloc(sizeof(char)*strlen(projectName)+17);
	strcpy(sendStr, "currentversion:");
	strcat(sendStr, projectName);
	//printf("sendStr is %s\n", sendStr);
	int sockfd = connectServer();
	write(sockfd, sendStr, strlen(sendStr));
	int len = 0;
	int size = 0;
	int timeouts = 0;
	while (!len && ioctl (sockfd,FIONREAD,&len) >= 0){
    	sleep(1);
    	timeouts++;
		if (timeouts == 5){
			printf("Error no response from server\n");
			close(sockfd);
			exit(1);
		}
    }
    char buff[len+1]; 
	if (len > 0) {
  		len = read(sockfd, buff, len);
  		size += len;
	}
	//printf("size is %i\n", size);
	if (strcmp(buff, "exit") == 0){
		printf("Error. Project or Manifest does not exist on the server\n");
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
		if (linesize < 10){ //skips first line
			if (firstline == 0){	//gets version of manifest on first line
				//char* totalVersion = (char*)malloc(sizeof(char)*linesize+1);
				//memcpy(totalVersion, &buff[tracker-linesize], linesize);
				//totalVersion[linesize] = '\0';
				//serverVersion = atoi(totalVersion);
				//printf("totalVersion is %i\n", clientVersion);
				firstline++;
			}
			tracker++;
			continue;	//line doces not have contain a filename
		}
		//char* line = (char*)malloc(sizeof(char)*linesize+1);
		//memcpy(line, &buff[tracker-linesize], linesize);
		//line[linesize] = '\0';
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
		//char* hash = (char*)malloc(sizeof(char)*41);
		//memcpy(hash, &buff[tracker-linesize+strlen(version)+strlen(fileName)+4], 40);
		//hash[41] = '\0';
		printf("%s ", fileName);
		printf("%s\n", version);
		//printf("hash is %s\n", hash);
		//addManifestData(fileName, hash, ver, 1);
		free(fileName);
		//free(hash);
		free(version);
	}
	close(sockfd);
}

void addUpdateData(char* fileName, short flag){
	if (ud == NULL){
		ud = (updateData*)malloc(sizeof(updateData));
		ud->fileName = (char*)malloc(sizeof(char)*strlen(fileName)+1);
		strcpy(ud->fileName, fileName);
		ud->flag = flag;
		ud->next = NULL;
	}
	else{
		updateData* udptr = ud;
		while (udptr->next != NULL){
			udptr = udptr->next;
		}
		updateData* tmp = (updateData*)malloc(sizeof(updateData));
		tmp->fileName = (char*)malloc(sizeof(char)*strlen(fileName)+1);
		strcpy(tmp->fileName, fileName);
		tmp->flag = flag;
		tmp->next = NULL;
		udptr->next = tmp;
		//printf("%s %i\n", udptr->next->fileName, udptr->next->flag);
	}
}

void getUpdateData(char* updatePath){
	int fileptr = open(updatePath, O_RDONLY);
	int currentPos = lseek(fileptr, 0, SEEK_CUR);
	int size = lseek(fileptr, 0, SEEK_END);    //get length of file
	lseek(fileptr, currentPos, SEEK_SET);  //set position back to start
	char c[size+1];
	int tracker = 0;
	int linesize = 0;
	if(read(fileptr, c, size) != 0){
		while (tracker < size){
			linesize = 0;
			while (c[tracker] != '\n'){
				tracker++;
				linesize++;
			}
			if (linesize < 10){ //skips first line
				tracker++;
				continue;	//line doces not have contain a filename
			}
			char* line = (char*)malloc(sizeof(char)*linesize+1);
			memcpy(line, &c[tracker-linesize], linesize);
			line[linesize] = '\0';
			//printf("line is %s\n", line);
			char* updateFlag = (char*)malloc(sizeof(char)*2);
			memcpy(updateFlag, &c[tracker-linesize], 1);
			updateFlag[1] = '\0';
			int fileLength = linesize - 2; 
			char* fileName = (char*)malloc(sizeof(char)*fileLength+1);
			memcpy(fileName, &c[tracker-linesize+2], fileLength);
			fileName[fileLength] = '\0';
			//printf("fileName is %s\n", fileName);
			//printf("flag is %s\n", updateFlag);
			if (strcmp(updateFlag, "M") == 0){
				addUpdateData(fileName, 1);
			}
			else if (strcmp(updateFlag, "A") == 0){
				addUpdateData(fileName, 2);
			}
			else if (strcmp(updateFlag, "D") == 0){
				addUpdateData(fileName, 3);
			}
			free(updateFlag);
			free(fileName);
		}
	}
	close(fileptr);
}

void upgradeDelete(){
	manifestData* mdLivePtr = livemd;
	manifestData* mdClientPtr = clientmd;
	
	while (mdLivePtr != NULL){
		while (mdClientPtr != NULL){
			if (mdClientPtr == NULL){
				break;
			}
			if (strcmp(mdClientPtr->fileName, livemd->fileName) == 0){
				if (livemd->flag == 3){
					mdClientPtr->flag = 3;
				}
			}
			mdClientPtr = mdClientPtr->next;
		}
		mdLivePtr = mdLivePtr->next;
	}
	
}

void upgradeUM(char* projectName){
	updateData* udPtr = ud;
	int length = 0;
	while (udPtr != NULL){
		length = length + strlen(udPtr->fileName)+2;	//+2 for : and \0
		udPtr = udPtr->next;
	}
	//printf("length is %i\n", length);
	char* sendStr = (char*)malloc(sizeof(char)*(strlen(projectName)*2)+23+length);
	strcpy(sendStr, "upgrade:");
	strcat(sendStr, projectName);	//a token will be projectName so server knows the projectName
	strcat(sendStr, ":");
	strcat(sendStr, projectName);
	strcat(sendStr, "/.Manifest:");
	//printf("send str %s\n", sendStr);
	
	udPtr = ud;
	while (udPtr != NULL){
		if (udPtr->flag == 1 || udPtr->flag == 2){
			//printf("yes\n");
			strcat(sendStr, udPtr->fileName);
			strcat(sendStr, ":");
			//printf("sendstr is %s\n", sendStr);
		}
		udPtr = udPtr->next;
	}
	int sockfd = connectServer();
	write(sockfd, sendStr, strlen(sendStr));
	int len = 0;
	int size = 0;
	int timeout = 0;
	while (!len && ioctl (sockfd,FIONREAD,&len) >= 0){
    	sleep(1);
    	timeout++;
    	if (timeout == 5){
    		printf("Error no response from server\n");
    		close(sockfd);
    		exit(1);
    	}
    }
    char buff[len+1]; 
	if (len > 0) {
  		len = read(sockfd, buff, len);
  		size += len;
	}
	//printf("size is %i\n", size);
	if (strcmp(buff, "exit") == 0){
		printf("Project does not exist on the server\n");
		exit(1);
	}
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
}

void upgrade(char* projectName){
	DIR* dir = opendir(projectName);
    if (dir){ //directory exists
		char* updatePath = (char*)malloc(sizeof(char)*(strlen(projectName)+9));
		updatePath = strcpy(updatePath, projectName);
		updatePath = strcat(updatePath, "/.Update");
		int fileptr = open(updatePath, O_RDONLY);
		if(fileptr == -1){
			printf("Update file doesn't exist. Please perform an update\n");
			exit(1);
		}
		else{
			int currentPos = lseek(fileptr, 0, SEEK_CUR);
			int size = lseek(fileptr, 0, SEEK_END);	//get length of file
			lseek(fileptr, currentPos, SEEK_SET);  //set position back to start
			char c[size+1];
			if(size == 0){
				printf("Project is already up to date\n");
				close(fileptr);
				remove(updatePath);
				exit(1);
   			}
   			close(fileptr);
   			getUpdateData(updatePath);
   			getClientManifestData(projectName);
   			getLiveManifestData(projectName);
   			//upgradeDelete();
   			upgradeUM(projectName);
   			remove(updatePath);
		}
	}
	else if (ENOENT == errno){	//directory doesn't exist
		printf("Client side does not have local copy of project\n");
	}		
}

void addCommitData(char* fileName, char* hash, int version, short flag){
	if (commitmd == NULL){
		commitmd = (manifestData*)malloc(sizeof(manifestData));
		commitmd->fileName = (char*)malloc(sizeof(char)*strlen(fileName)+1);
		strcpy(commitmd->fileName, fileName);
		commitmd->hash = (char*)malloc(sizeof(char)*41);
		strcpy(commitmd->hash, hash);
		commitmd->version = version;
		commitmd->flag = flag;
		commitmd->next = NULL;
		//printf("server fileName %s\n", commitmd->fileName);
		//printf("server hash %s\n", commitmd->hash);
		//printf("server version %i\n", commitmd->version);
	}
	else{
		manifestData* mdptr = commitmd;
		while (mdptr->next != NULL){
			mdptr = mdptr->next;
		}
		manifestData* tmp = (manifestData*)malloc(sizeof(manifestData));
		tmp->fileName = (char*)malloc(sizeof(char)*strlen(fileName)+1);
		strcpy(tmp->fileName, fileName);
		tmp ->hash = (char*)malloc(sizeof(char)*41);
		strcpy(tmp->hash, hash);
		tmp->version = version;
		tmp->flag = flag;
		tmp->next = NULL;
		mdptr->next = tmp;
		//printf("s fileName %s\n", mdptr->next->fileName);
		//printf("s hash %s\n", mdptr->next->hash);
		//printf("s version %i\n", mdptr->next->version);
		//printf("s flag %i\n", mdptr->next->flag);
	}
		
}
void getCommitData(char* projectName){
	char* commitPath = (char*)malloc(sizeof(char)*(strlen(projectName)+10));
	commitPath = strcpy(commitPath, projectName);
	commitPath = strcat(commitPath, "/.Commit");
	int commitFile = open(commitPath, O_RDWR);
	if (commitFile == -1){
		printf(".Commit doesn't exist on the client\n");
		exit(1);
	}
	int currentPos = lseek(commitFile, 0, SEEK_CUR);
	int size = lseek(commitFile, 0, SEEK_END);    //get length of file
	lseek(commitFile, currentPos, SEEK_SET);  //set position back to start
	char c[size+1];
	int tracker = 0;
	int linesize = 0;
	int firstline = 0;
	if(read(commitFile, c, size) != 0){
		while (tracker < size){
			linesize = 0;
			while (c[tracker] != '\n'){
				tracker++;
				linesize++;
			}
			if (linesize < 10){ //skips first line
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
			char* updateFlag = (char*)malloc(sizeof(char)*2);
			memcpy(updateFlag, &c[tracker-linesize], 1);
			updateFlag[1] = '\0';
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
			//printf("version is %s\n", version);
			//printf("update flag is %s\n", updateFlag);
			if (strcmp(updateFlag, "U") == 0){
				addCommitData(fileName, hash, ver, 0);
			}
			else if (strcmp(updateFlag, "A") == 0){
				addCommitData(fileName, hash, ver, 2);
			}
			else if (strcmp(updateFlag, "D") == 0){
				addCommitData(fileName, hash, ver, 3);
			}
			free(fileName);
			free(hash);
			free(version);
			free(updateFlag);
		}
	}
	close(commitFile);
}

void push(char* projectName){
      char* commitPath = (char*)malloc(sizeof(char)*(strlen(projectName)+10));
      commitPath = strcpy(commitPath, projectName);
      commitPath = strcat(commitPath, "/.Commit");
      int commitFile = open(commitPath, O_RDWR);
      if (commitFile == -1){
      	printf(".Commit doesn't exist on the client\n");
      	exit(1);
      }
      
      getClientManifestData(projectName);	//for client version #
      getServerManifestData(projectName);
      
      
      
      if (serverVersion != clientVersion){
      	printf("Server and client manifest version do not match. Please perform an update+upgrade\n");
      	remove(commitPath);
      	exit(1);
      }
      
      DIR* dir = opendir(projectName);
      if (dir){ //directory exists
		  char* updatePath = (char*)malloc(sizeof(char)*(strlen(projectName)+9));
		  updatePath = strcpy(updatePath, projectName);
		  updatePath = strcat(updatePath, "/.Update");
		  int file = open(updatePath, O_RDONLY);
		  if(file == -1){ //update file doesn't exist
		  }
		  else{ //if update file exists, check if empty
			  int currentPos = lseek(file, 0, SEEK_CUR);
			  int size = lseek(file, 0, SEEK_END); //get length of file
			  if(size != 0){
			  	printf("There are pending updates need to be made. Perform an upgrade first\n");
			  	exit(0);
			  }
		  }
		  close(file);
		  getCommitData(projectName);
		  manifestData* mdptr = commitmd;
		  int length = 0;
		  while (mdptr != NULL){	// gets length for all file names in .Commit
		  	length = length + strlen(mdptr->fileName)+2;	//+2 for space and \0
		  	mdptr = mdptr->next;
		  }
		  //printf("length is %i\n", length);
		  char* tarFiles = (char*)malloc(sizeof(char)* (strlen(projectName)*2 + 26 + length));
		  strcpy(tarFiles, "tar cfz ");
		  strcat(tarFiles, projectName);
		  strcat(tarFiles, ".tgz ");
		  strcat(tarFiles, projectName);
		  strcat(tarFiles, "/.Commit ");
		  //strcat(tarFiles, projectName);
		  mdptr = commitmd;
		  while (mdptr != NULL){
			strcat(tarFiles, mdptr->fileName);
			strcat(tarFiles, " ");
			//printf("tarFiles is %s\n", tarFiles);
			mdptr = mdptr->next;
		}
		//printf("tarFiles is %s\n", tarFiles);
		//printf("tarFiles is %s\n", tarFiles);
		system(tarFiles);
		free(tarFiles);
		char* sendStr = (char*)malloc(sizeof(char)*strlen(projectName)+7);
		strcpy(sendStr, "push:");
		strcat(sendStr, projectName);
		//printf("sendStr is %s\n", sendStr);
		int sockfd = connectServer();
		/*int n = 0;
		fd1set set;
		struct timeval timeout;
		FD_ZERO(&set);
		FD_SET(sockfd, &set);
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		//select(FD_SETSIZE, &set, NULL, NULL, &timeout);
		//select(FD_SETSIZE, &set, NULL, NULL, &timeout);
		select(FD_SETSIZE, &set, NULL, NULL, &timeout);
		*/
		write(sockfd, sendStr, strlen(sendStr));
		sleep(1);
		
    	char* tarFile = (char*)malloc(sizeof(char)*(strlen(projectName)+5));	//for opening tarFile
    	strcpy(tarFile, projectName);
    	strcat(tarFile, ".tgz");
    	tarFile[strlen(projectName)+4] = '\0';
    	//printf("tarFile is %s\n", tarFile);
    	int fileptr = open(tarFile, O_RDONLY);
		if(fileptr == -1){
			printf("cannot open compressed file \n");
			exit(1);
		}
		else{
			//printf("opened tar\n");
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
			write(sockfd, c, size);
		}
		remove(tarFile);
		
		int len = 0;
		size = 0;
		int timeouts = 0;
		while (!len && ioctl (sockfd,FIONREAD,&len) >= 0){
			//printf("ok\n");
    		sleep(1);
    		timeouts++;
    		if (timeouts == 9){
    			printf("Error no response from server\n");
    			remove(commitPath);
    			close(sockfd);
    			exit(1);
    		}
   	 	}
    	char buff[len+1]; 
		if (len > 0) {
  			len = read(sockfd, buff, len);
  			size += len;
		}
		//printf("size is %i\n", size);
		if (strcmp(buff, "exit") == 0){
			printf("Push failed, check if commit exists on server\n");
			remove(commitPath);
			exit(1);
		}
		else{
			char* oldManifest = (char*)malloc(sizeof(char)*strlen(projectName)+11);
			strcpy(oldManifest, projectName);
			strcat(oldManifest, "/.Manifest");
			//printf("old manifest is %s\n", oldManifest);
			remove(oldManifest);
			int newManifestFile = creat(oldManifest, O_APPEND | O_RDWR | 0600);
			write(newManifestFile, buff, len);
			close(newManifestFile);
			remove(commitPath);
		}
	}
	else if (ENOENT == errno){    //directory doesn't exist
		printf("Client side does not have local copy of project.\n");
	}
}

void DeleteAll(char* pathorfile){ //implements recursive function to delete all files and subdirectories of server folder
   //printf("Current path: %s\n", pathorfile);
   struct dirent *pDirent;
   DIR *pDir;
   pDir = opendir(pathorfile);
   if(pDir == NULL){
       //printf("Cannot open directory '%s'\n", pathorfile);
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
           DeleteAll(new);
       }
       else if(S_ISREG(path_stat.st_mode)){ //is a file
           //printf("File: %s\n", new);
	   remove(new);
       }
   }
   closedir(pDir);
   rmdir(pathorfile);
   return;
}

void rollback(char* projectName, char* version){
	int ver = atoi(version);
	//printf("ver is %i\n", ver);
	if (ver < 1){	//lowest version is 1
		printf("Invalid version number\n");
		exit(1);
	}
	char* sendStr = (char*)malloc(sizeof(char)*(strlen(projectName)+strlen(version)+14));
	strcpy(sendStr, "rollback:");
	strcat(sendStr, projectName);
	strcat(sendStr, ":");
	strcat(sendStr, version);
	
	int sockfd = connectServer();
	write(sockfd, sendStr, strlen(sendStr));
	free(sendStr);
	
	int len = 0;
	int size = 0;
	int timeout = 0;
	while (!len && ioctl (sockfd,FIONREAD,&len) >= 0){
    	sleep(1);
    	timeout++;
    	if (timeout == 5){
    		printf("Error no response from server\n");
    		close(sockfd);
    		exit(1);
    	}
    }
    char buff[len+1]; 
	if (len > 0) {
  		len = read(sockfd, buff, len);
  		size += len;
	}
	//printf("size is %i\n", size);
	if (strcmp(buff, "exit") == 0){
		printf("Rollback version or project does not exist on server\n");
		exit(1);
	}
}

void history(char* projectName){
	char* sendStr = (char*)malloc(sizeof(char)*strlen(projectName)+10);
	strcpy(sendStr, "history:");
	strcat(sendStr, projectName);
	
	int sockfd  = connectServer();
	write(sockfd, sendStr, strlen(sendStr)+1);
	int len = 0;
	int size = 0;
	int timeouts = 0;
	while (!len && ioctl (sockfd,FIONREAD,&len) >= 0){
    	sleep(1);
    	timeouts++;
		if (timeouts == 5){
			printf("Erorr. Server timed out\n");
			close(sockfd);
			exit(1);
		}
    }
    char buff[len+1]; 
	if (len > 0) {
  		len = read(sockfd, buff, len);
  		size += len;
	}
	//printf("size is %i\n", size);
	if (strcmp(buff, "exit") == 0){
		printf("Error. Project or .History does not exist on the server\n");
		exit(1);
	}
	printf("%s", buff);
	free(sendStr);
}

int main(int argc, char *argv[]){
	if (argc < 2 || argc > 4){
		printf("Incorrect number of arguments\n");
		exit(1);
	}
	if (strcmp(argv[1],"configure") == 0){
		if (argc != 4){
			printf("Incorrect number of arguments for configure\n");
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
	}
	else if (strcmp(argv[1], "destroy")== 0){
		if (argc !=3){
			printf("Incorrect number of arguments for destroy\n");
			exit(1);
		}
		destroy(argv[2]);
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
	}
	else if (strcmp(argv[1], "commit") == 0){
		if (argc != 3){
			printf("Incorrect number of arguments for commit\n");
			exit(1);
		}
		commit(argv[2]);
	}
	else if (strcmp(argv[1], "currentversion") == 0){
		if (argc !=3 ){
			printf("Incorrect number of arguments for currentversion\n");
			exit(1);
		}
		currentversion(argv[2]);
	}
	else if (strcmp(argv[1], "upgrade") == 0){
		if (argc != 3){
			printf("Incorrect number of arguments for currentversion\n");
			exit(1);
		}
		upgrade(argv[2]);
	}
	else if (strcmp(argv[1], "push") == 0){
		if (argc != 3){
			printf("Inorrect number of arguments for currentversion\n");
			exit(1);
		}
		push(argv[2]);
	}
	else if (strcmp(argv[1], "rollback") == 0){
		if (argc!= 4){
			printf("Incorrect number of arguments for rollback\n");
			exit(1);
		}
		rollback(argv[2], argv[3]);
	}
	else if (strcmp(argv[1], "history") == 0){
		if (argc!= 3){
			printf("Incorrect number of arguments for history\n");
			exit(1);
		}
		history(argv[2]);
	}
	else{
		printf("Please enter a proper command\n");
		exit(1);
	}
	printf("Client command completed\n");
	return 0;
}
