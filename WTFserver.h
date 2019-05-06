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
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <openssl/sha.h>
#include <time.h>
#define MAX 80 
#define SA struct sockaddr

typedef struct manifestData{
	char* fileName;
	char* hash;
	short flag; // U = 0 M = 1 A = 2 D = 3
	int version; ////individual file version number for manifest
	struct manifestData* next;
}manifestData;

typedef struct fileList{	//linked list of file data
	int length;
	int size;
	char* name;
	char* data;
	struct fileList* next;
}fileList;

manifestData* commitmd = NULL; //linked list for commit manifest data
manifestData* md = NULL; //linked list for client manifest data
fileList* fl = NULL;

char* createDigest(char* fileName, char* digest);
void DeleteAll(char* pathorfile); //implements recursive function to delete all files and subdirectories of server folder
void createHistory(char* projectName);
int checkCommits(char* commitsDir, char* digest); //checks .Commits folder for equal digest. return 1 for success 0 for failure
void addCommitData(char* fileName, char* hash, int version, short flag);
int getCommitData(char* commitPath);		//return 0 if commit doesnt exist
void createNewManifest(int manifestFile, char* newManifest, char* manifestName);
void createBackup(char* projectName);
int getHistoryVersion(char* projectName);
void pushHistory(char* commitPath, char* projectName);

void create(char* token, int sockfd);
void destroy(char* token);
void checkout(char* token, int sockfd);
void update(char* token, int sockfd);	//send manifest to client
void upgrade(char* token, int sockfd, char* copy);
void commit(char* token, int sockfd);
void push(char* token, int sockfd);
void currentversion(char* token, int sockfd);
void rollback(char* token, int sockfd);
void history(char* token, int sockfd);
void *func(void* vptr_sockfd);
