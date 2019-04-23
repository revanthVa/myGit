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
#define MAX 80 
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
		printf("connection failed\n");
		exit(1);
	}
	else
		printf("connected to the server\n");
		return sockfd;
}
void configure(char* ip, char* port, int argc){ //create file with IP and port of server
		if (argc != 4){
			printf("Incorrect number of arguments for configure");
			exit(1);
		}
		int conf;
		conf = creat("configure", O_APPEND | O_WRONLY | 0600);
		printf("yes");
		int len = strlen(ip) + strlen(port) + 3;
		char *ip_port = (char*) malloc(len);
		ip_port = strcpy(ip_port, ip);
		ip_port = strcat(ip_port, "\n"); //seperate IP and port with \n
		ip_port = strcat(ip_port, port);
		ip_port = strcat(ip_port, "\n");
		write(conf, ip_port, strlen(ip_port));
}
void create(char* name, int argc){
	if (argc !=3){
		printf("Incorrect number of arguments for create\n");
		exit(1);
	}
	char *str = (char*)malloc(sizeof(char)*(strlen(name)+9));
	str = strcpy(str, "create ");
	str = strcat(str, name);
	//printf("str is %s\n", str);
	int sockfd = connectServer();
	write(sockfd, str, strlen(str)+1);
	close(sockfd);
}
int main(int argc, char *argv[]){
	if (strcmp(argv[1],"configure") == 0){
		configure(argv[2], argv[3], argc);
		//int sockfd = connectServer();
		//func(sockfd);
		//close(sockfd);
	}
	else if (strcmp(argv[1], "create" )== 0){
		create(argv[2], argc);
		//printf("create\n");
	}
}
