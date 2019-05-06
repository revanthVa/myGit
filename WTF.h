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
#include <sys/ioctl.h>
#include <dirent.h>
#include <errno.h>
#include <openssl/sha.h>
#include <sys/ioctl.h>
#include <time.h>
#define MAX 80

typedef struct manifestData{ //linked list for manifest data
	char* fileName;
	char* hash;
	short flag; // U = 0 M = 1 A = 2 D = 3
	int version; ////individual file version number for manifest
	struct manifestData* next;
}manifestData;

typedef struct updateData{
	char* fileName;
	short flag;	// U = 0 M = 1 A = 2 D = 3
	struct updateData* next;
}updateData;

manifestData* servermd = NULL; //linked list for server manifest data
manifestData* clientmd = NULL; //linked list for client manifest data
manifestData* livemd = NULL; //linked list for live manifest data
manifestData* commitmd = NULL; //linked list for commit manifest data
updateData* ud = NULL; //linked list for the .Update

int serverVersion = -1; //server's manifest version number
int clientVersion = -1; //client's manifest version number

int connectServer(); //gets ip and port from file and connects
void configure(char* ip, char* port); //create file with IP and port of server

char* createDigest(char* fileName, char* digest);
void checkAdd(char* fileName, char* dirName, char* digest);	//check if file is already in manifest and update if it is.
void DeleteAll(char* pathorfile); //implements recursive function to delete all files and subdirectories of server folder

void addManifestData(char* fileName, char* hash, int version, int clientOrServer); //adds to manifestData struct. 0 is client 1 is server.
void addLiveManifestData(char* fileName, char* hash);
void getLiveManifestData(char* pathorfile);	//goes through all subdirectories and finds files and gets the hashes for the files.
void getClientManifestData(char* projectName);
void getServerManifestData(char* projectName);
void printManifestData(); //prints all of client, server, and live manifest data

void updateUpload(int updateFile);
void updateModify(int updateFile); //file both server and client have, but file version different and the clients live hash is same as its manifest
void updateAdd(int updateFile);	//file in servers manifest but not in clients
void updateDelete(int updateFile);
int updateConflicts();	//manifest different, file version different and client live hash different
void addUpdateData(char* fileName, short flag);
void getUpdateData(char* updatePath);

void upgradeDelete();
void upgradeUM(char* projectName);

void addCommitData(char* fileName, char* hash, int version, short flag);
void getCommitData(char* projectName);

void create(char* name);
void destroy(char* name);
void add(char* dirName, char* fileName);
void removeFile(char* dirName, char* fileName);
void checkout(char* projectName);
void update(char* projectName);
void upgrade(char* projectName);
void commit(char* projectName);
void push(char* projectName);
void currentversion(char* projectName);
void rollback(char* projectName, char* version);
void history(char* projectName);
