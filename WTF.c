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
	digest = (char*)malloc(sizeof(char)*(SHA_DIGEST_LENGTH*2));
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
	char* digest;
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
			printf("incorrect number of arguments for checkout");
			exit(1);
		}
		//int newFile = creat("test2/testee.txt", O_APPEND | O_WRONLY | 0600);
		//if (newFile == -1){
			//printf("could not create file\n");
		//}
		checkout(argv[2]);
	}
	printf("Client command completed.\n");
}
