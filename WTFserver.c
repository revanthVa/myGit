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
#include <sys/ioctl.h>
#include <openssl/sha.h>
#include <time.h>
#define MAX 80 
#define SA struct sockaddr 

//gcc WTFserver.c -lpthread -o WTFserver

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

manifestData* commitmd = NULL;
manifestData* md = NULL;
fileList* fl = NULL;

char* createDigest(char* fileName, char* digest){
	int file = open(fileName, O_RDONLY);
	if (file == -1){
	    printf("File does not exist\n");
	    //exit(1);
	}
	else{
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
}

/*void getFileData(char* fileName){
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
*/

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

void createHistory(char* projectName){
	char* historyPath = (char*)malloc(sizeof(char)*strlen(projectName)+11);
	strcpy(historyPath, projectName);
	strcat(historyPath, "/.History");
	int historyFile = creat(historyPath, O_WRONLY | 0600);
	write(historyFile, "create\n1\n", 9);
	close(historyPath);
	free(historyPath);
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
   		createHistory(token);
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
    	char* versionsFolder = (char*)malloc(sizeof(char)*strlen(token)+12);
    	strcpy(versionsFolder, token);
    	strcat(versionsFolder, "/.Versions");
    	char* commitsFolder = (char*)malloc(sizeof(char)*strlen(token)+11);
    	strcpy(commitsFolder, token);
    	strcat(commitsFolder, "/.Commits");
    	int malloclength = (strlen(token)*2)+strlen(versionsFolder)+strlen(commitsFolder)+48;
    	char* systemStr = (char*)malloc(sizeof(char)*malloclength);
    	strcpy(systemStr, "tar cfz ");
    	strcat(systemStr, token);
    	strcat(systemStr, ".tgz ");
    	strcat(systemStr, "--exclude='");
		strcat(systemStr, versionsFolder);
		strcat(systemStr, "' ");
		strcat(systemStr, "--exclude='");
		strcat(systemStr, commitsFolder);
		strcat(systemStr, "' ");
    	strcat(systemStr, token);
    	
    	system(systemStr);
    	free(versionsFolder);
    	free(commitsFolder);
    	free(systemStr);
    	
    	char* tarFile = (char*)malloc(sizeof(char)*(strlen(token)+5));	//for open tarFile
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
			//printf("copy[i] %c ", copy[i]);
			tracker++;
			i++;
		}
		projectDir = realloc(projectDir, (sizeof(char)*tracker+1));
		memcpy(projectDir, &copy[i-tracker], tracker);
		projectDir[tracker] = '\0';
		//printf("projectDir is %s\n", projectDir);
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
		//printf("createTar is %s\n", createTar);
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
				//printf("projectName is %s\n", projectName);
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
		//printf("tar files is %s\n", tarFiles);
		//printf("l is %i\n", l);
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

int checkCommits(char* commitsDir, char* digest){ //checks .Commits folder for equal digest. return 1 for success 0 for failure
DIR* dir;
struct dirent *ent;
if ((dir = opendir(commitsDir)) != NULL){
	while ((ent = readdir(dir)) != NULL){
		if (ent->d_type == DT_REG){		//is a regular file
			//printf("dp stuff\n");
			char* newDigest = (char*)malloc(sizeof(char)*41);
			newDigest[41] = '\0';
			//printf("ent name is %s\n",ent->d_name);
			char* commitDigestPath = (char*)malloc(sizeof(char)*(strlen(commitsDir)+strlen(ent->d_name)+4));
			strcpy(commitDigestPath, commitsDir);
			strcat(commitDigestPath, "/");
			strcat(commitDigestPath, ent->d_name);
			newDigest = createDigest(commitDigestPath, digest);
			//printf("new digest is %s\n",  newDigest);
			if (strcmp(digest, newDigest) == 0){
				//printf("they are equal\n");
				free(newDigest);
				free(commitDigestPath);
				return 1;
			}
			free(newDigest);
			free(commitDigestPath);
		}
	}
	closedir(dir);
}
else{
	return 0; //failed because directory doesn't exist
}

return 0;
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

int getCommitData(char* commitPath){		//return 0 if commit doesnt exist
	int commitFile = open(commitPath, O_RDWR);
	if (commitFile == -1){
		printf(".Commit doesn't exist on the server\n");
		return 0;
	}
	int currentPos = lseek(commitFile, 0, SEEK_CUR);
	int size = lseek(commitFile, 0, SEEK_END);    //get length of file
	lseek(commitFile, currentPos, SEEK_SET);  //set position back to start
	char c[size+1];
	int tracker = 0;
	int linesize = 0;
	if(read(commitFile, c, size) != 0){
		while (tracker < size){
			linesize = 0;
			while (c[tracker] != '\n'){
				tracker++;
				linesize++;
			}
			if (linesize <10){ //skips first line
				tracker++;
				continue;	//line does not have contain a filename
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
	//printf("commit path is %s\n", commitPath);
	remove(commitPath);
	return 1;
}

void createNewManifest(int manifestFile, char* newManifest, char* manifestName){
  int newManifestFile = creat(newManifest, O_APPEND | O_RDWR | 0600);
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
	  //printf("linesize is 4asdf %i\n", linesize);
	  //printf("total version is ssfd %s\n", totalVersion);
	  int totalVersionNum = atoi(totalVersion);
	  totalVersionNum++;	//increase new manifest version by 1
	  int verlength = snprintf( NULL, 0, "%d", totalVersionNum);
	  char strVer[verlength+1];	//updated version number
	  strVer[verlength+1] = '\0';
	  sprintf(strVer, "%d", totalVersionNum); //convert int to string
	  //printf("totalVersion is %i\n", clientVersion);
	  firstline++;
	  write(newManifestFile, strVer, verlength);
	  write(newManifestFile, "\n", 1);
	  free(totalVersion);
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
	  exit(1);			//this should never happen
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
	  
	  manifestData* mdptr = commitmd;
	  while (mdptr != NULL && strcmp(mdptr->fileName, fileName) != 0){
	    mdptr = mdptr->next;
	  }
	  if (mdptr != NULL){
	    int verlength = snprintf( NULL, 0, "%d", mdptr->version );
	    //printf("verlength is %i\n", verlength);
	    char strVer[verlength+1];	//updated version number
	    strVer[verlength+1] = '\0';
	  sprintf(strVer, "%d", mdptr->version); //convert int to string
	  if (mdptr->flag == 0){	// U = 0
	    //printf("matching commit and manifest\n");
	    //printf("version is %i\n", mdptr->version);
	    write(newManifestFile, "  ", 2);
	    write(newManifestFile, strVer, verlength);
	    write(newManifestFile, " ", 1);
	    write(newManifestFile, mdptr->fileName, strlen(mdptr->fileName));
	    write(newManifestFile, " ", 1);
	    write(newManifestFile, mdptr->hash, strlen(mdptr->hash));
	    write(newManifestFile, "\n", 1);
	    continue;
	  }
	  else if (mdptr->flag == 3){		//D == 2
	    continue;
	  }
	  }
	  write(newManifestFile, "  ", 2);
	  write(newManifestFile, version, strlen(version));
	  write(newManifestFile, " ", 1);
	  write(newManifestFile, fileName, strlen(fileName));
	  write(newManifestFile, " ", 1);
	  write(newManifestFile, hash, strlen(hash));
	  write(newManifestFile, "\n", 1);
	  //printf("fileName is %s\n", fileName);
	  //printf("hash is %s\n", hash);
	  free(fileName);
	  free(hash);
	  free(version);
    }
  }
  close(manifestFile);
  
  manifestData* mdptr = commitmd;
  while (mdptr != NULL){
  	if (mdptr == NULL){
  		break;
  	}
  	else if (mdptr->flag == 2){	//A = 2
  		int verlength = snprintf( NULL, 0, "%d", mdptr->version );
	    //printf("verlength is %i\n", verlength);
	    char strVer[verlength+1];	//updated version number
	    strVer[verlength+1] = '\0';
	    sprintf(strVer, "%d", mdptr->version); //convert int to string
  		write(newManifestFile, "  ", 2);
	    write(newManifestFile, strVer, verlength);
	    write(newManifestFile, " ", 1);
	    write(newManifestFile, mdptr->fileName, strlen(mdptr->fileName));
	    write(newManifestFile, " ", 1);
	    write(newManifestFile, mdptr->hash, strlen(mdptr->hash));
	    write(newManifestFile, "\n", 1);
  	}
    mdptr = mdptr->next;
  }
  close(newManifestFile);
  remove(manifestName);
  rename(newManifest,  manifestName);
}

void createBackup(char* projectName){
	char* versionsFolder = (char*)malloc(sizeof(char)*strlen(projectName)+12);
	strcpy(versionsFolder, projectName);
	strcat(versionsFolder, "/.Versions");
	
	DIR* dir;
	struct dirent * entry;
	dir = opendir(versionsFolder);
	if(ENOENT == errno){ //no .Versions folder yet, so make mkdir()
		mkdir(versionsFolder, 0700);
		char* backupName = (char*)malloc(sizeof(char)*(strlen(versionsFolder)+strlen(projectName)+10));
		strcpy(backupName, versionsFolder);
		strcat(backupName, "/");
		strcat(backupName, projectName);
		strcat(backupName, "1.tgz");
		
		char* createTar = (char*)malloc(sizeof(char)*strlen(backupName)+strlen(projectName)+13);
		strcpy(createTar, "tar cfz ");
		strcat(createTar, backupName);
		strcat(createTar, " ");
		strcat(createTar, projectName);
		//printf("the backups folder name is %s\n", createTar);
		system(createTar);
		free(backupName);
		free(createTar);
		free(versionsFolder);
	}
	else{
		//directory exists
		int fileCount = 1;	//starts at 1 for new backup name
		while ((entry = readdir(dir)) != NULL) {
    		if (entry->d_type == DT_REG) { // If the entry is a regular file 
         		fileCount++;
    		}
		}
		
		int fileCountLength = snprintf( NULL, 0, "%d", fileCount );
	    char strVer[fileCountLength+1];	
	    strVer[fileCountLength+1] = '\0';
	    sprintf(strVer, "%d", fileCount); //convert int to string
	  
		char* backupName = (char*)malloc(sizeof(char)*(strlen(versionsFolder)+strlen(projectName)+strlen(strVer)+10));
		strcpy(backupName, versionsFolder);
		strcat(backupName, "/");
		strcat(backupName, projectName);
		strcat(backupName, strVer);
		strcat(backupName,".tgz");
		char* createTar = (char*)malloc(sizeof(char)*strlen(backupName)+strlen(projectName)+strlen(versionsFolder)+13);
		strcpy(createTar, "tar cfz ");
		strcat(createTar, backupName);
		strcat(createTar, " ");
		strcat(createTar, projectName);
		//printf("the backups folder name is %s\n", createTar);
		system(createTar);
		free(backupName);
		free(createTar);
		free(versionsFolder);
		
	}
}

int getHistoryVersion(char* projectName){
	char* versionsPath = (char*)malloc(sizeof(char)*strlen(projectName)+12);
	strcpy(versionsPath, projectName);
	strcat(versionsPath, "/.Versions");
	DIR* dir;
	struct dirent * entry;
	dir = opendir(versionsPath);
	if(ENOENT == errno){ //no .Versions folder should not happen
	}
	else{
		int fileCount = 0;	//file count is version number for history
		while ((entry = readdir(dir)) != NULL) {
    		if (entry->d_type == DT_REG) { // If the entry is a regular file 
         		fileCount++;
    		}
		}
		return fileCount;
	}
}

void pushHistory(char* commitPath, char* projectName ){
	int commitFile = open(commitPath, O_RDONLY);
	char* manifestPath = (char*)malloc(sizeof(char)*strlen(projectName+12));
	strcpy(manifestPath, projectName);
	strcat(manifestPath, "/.Manifest");
	int manifestFile = open(manifestPath, O_RDONLY);

	char* historyPath = (char*)malloc(sizeof(char)*strlen(projectName)+11);
	strcpy(historyPath, projectName);
	strcat(historyPath, "/.History");
	int historyFile = open(historyPath, O_APPEND | O_RDWR);
	
	int currentPos = lseek(manifestFile, 0, SEEK_CUR);
	int size = lseek(manifestFile, 0, SEEK_END);    //get length of file
	lseek(manifestFile, currentPos, SEEK_SET);  //set position back to start
	char c[size+1];
	c[size+1] = '\0';
	//printf("size is %i\n", size);
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
			  		write(historyFile, "\n", 1);
			  		write(historyFile, totalVersion, strlen(totalVersion));
			  		write(historyFile, "\n", 1);
			  		free(totalVersion);
			  		break;
		  		}
		  	}
		}
	}
	
	
	currentPos = lseek(commitFile, 0, SEEK_CUR);
	size = lseek(commitFile, 0, SEEK_END);    //get length of file
	lseek(commitFile, currentPos, SEEK_SET);  //set position back to start
	c[size+1];
	c[size+1] = '\0';
	//printf("size is %i\n", size);
	tracker = 0;
	linesize = 0;
	if(read(commitFile, c, size) != 0){
		//printf("reading manifest\n");
	    write(historyFile, c, size);
	    write(historyFile, "\n", 1);
	}
	
	free(manifestPath);
	free(historyPath);
	close(historyFile);
	close(commitFile);
	close(manifestFile);
}

void push(char* token, int sockfd){
	token = strtok(NULL, ":");
	//printf("token is %s\n", token);
	char* projectName = (char*)malloc(sizeof(char)*strlen(token)+1);
	strcpy(projectName, token);
	int len = 0;
	int size = 0;
	int timeouts = 0;
	while (!len && ioctl (sockfd,FIONREAD,&len) >= 0){
    	sleep(1);
    	timeouts++;
    	if (timeouts == 5){
    		printf("Error no response from server\n");
    		write(sockfd, "exit", 5);
    		return;
    	}
    }
    char buff[len]; 
	if (len > 0) {
  		len = read(sockfd, buff, len);
	}
    //printf("len is %i\n", len);
    char* createTar = (char*)malloc(sizeof(char)*strlen(projectName)+6);
    strcpy(createTar, projectName);
    strcat(createTar, ".tgz");
    int newFile = creat(createTar, O_APPEND | O_RDWR | 0600);
    write(newFile, buff, len);
    close(newFile);
    char* commitPath = (char*)malloc(sizeof(char)*strlen(projectName)+11);
    strcpy(commitPath, projectName);
    strcat(commitPath, "/.Commit");
    char* untarCommit = (char*)malloc(sizeof(char*)*(strlen(createTar)+15+strlen(commitPath)));
   	strcpy(untarCommit, "tar -zxvf ");
   	strcat(untarCommit, createTar);
   	strcat(untarCommit, " ");
   	strcat(untarCommit, commitPath);
   	system(untarCommit);		//extract commit file from tar file
   	char* digest = (char*)malloc(sizeof(char)*41);
	digest[41] = '\0';
	digest = createDigest(commitPath, digest);
	//printf("digest %s\n", digest);
   	char* commitsDir = (char*)malloc(sizeof(char*)*strlen(projectName)+11);
   	strcpy(commitsDir, projectName);
   	strcat(commitsDir, "/.Commits");
   	int x = checkCommits(commitsDir, digest);
   	//printf("Commits dir is %s\n",commitsDir);
   	if (x == 0){	//same commit file was not found in commits folder
   		write(sockfd, "exit", 5);
   		remove(commitPath);
   		remove(createTar);
   		return;
   	}
   	else{	//success. replace the manifest
   		char* newManifest = (char*)malloc(sizeof(char)*strlen(projectName)+15);
   		strcpy(newManifest, projectName);
   		strcat(newManifest, "/.tmpmanifest");	//replace the server manifest with this after done rewriting
   		char* path = (char*)malloc(sizeof(char)*(strlen(projectName)+12));
   		strcpy(path, projectName);
   		strcat(path, "/.Manifest");
   		int manifestFile;
   		manifestFile = open(path, O_RDONLY);
   		if(manifestFile == -1){
   			printf("cannot open .Manifest\n");
   			write(sockfd, "exit", 5);
   			return;
   		}
   		else{
   			int x = getCommitData(commitPath);
   			if (x == 0){	//failed
   				write(sockfd, "exit", 5);
   				return;
   			}
   			createBackup(projectName);
   			DeleteAll(commitsDir);
   			
   			createNewManifest(manifestFile, newManifest, path);
   			char* untar = (char*)malloc(sizeof(char)*strlen(createTar)+11);
   			strcpy(untar, "tar -xvf ");
   			strcat(untar, createTar);
   			system(untar);
   			remove(createTar);
   			free(untar);
   			
   			pushHistory(commitPath, projectName);	//update the .History
   			remove(commitPath);
   			
   			manifestFile = open(path, O_RDONLY);	//send new manifest back to client
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
   		
   	}
   	free(commitsDir);
   	free(digest);
    free(projectName);
    free(createTar);
    free(commitPath);
    free(untarCommit);
}
// Function designed for chat between client and server. 

void rollback(char* token, int sockfd){
	int x = 0;
	token = strtok(NULL, ":");	//token is projectName now
	//printf("token is %s\n", token);	
	char* projectName = (char*)malloc(strlen(token)+1);
	strcpy(projectName, token);
	
	token = strtok(NULL, ":");	//token is rollback version now
	//printf("token is %s\n", token);
	
	char* versionPath = (char*)malloc(sizeof(char)*(strlen(projectName)*2+strlen(token)+20));
	strcpy(versionPath, projectName);
	strcat(versionPath, "/.Versions/");
	strcat(versionPath, projectName);
	strcat(versionPath, token);
	strcat(versionPath, ".tgz");
	
	char* newName = (char*)malloc(sizeof(char)*strlen(projectName)+6);
	strcpy(newName, projectName);
	strcat(newName, ".tgz");
	printf("newName is %s\n", newName);
	printf("version path is %s\n", versionPath);
	int fileptr = open(versionPath, O_RDONLY);		//send compressed vesion to client
	if(fileptr == -1){	//rollback version doesn't exist
		write(sockfd, "exit", 5);
		return;
	}
	
	//remove(newName);
	rename(versionPath, newName);
	DeleteAll(projectName);
	
	char* untar = (char*)malloc(sizeof(char)*strlen(projectName)+16);
	strcpy(untar, "tar -xvf ");
	strcat(untar, projectName);
	strcat(untar, ".tgz");
	system(untar);
	write(sockfd,"success", 8);
	
	remove(newName);
	free(newName);
	free(projectName);
	free(versionPath);
}

void history(char* token, int sockfd){
	token = strtok(NULL, ":");
	printf("token is %s\n", token);
	char* historyPath = (char*)malloc(sizeof(char)*strlen(token)+11);
	strcpy(historyPath, token);
	strcat(historyPath, "/.History");
	
	int historyFile = open(historyPath, O_RDONLY);
	if(historyFile == -1){
		free(historyPath);
		printf(".History does not exist\n");
	    write(sockfd, "exit", 5);
	    return;
	}
	int currentPos = lseek(historyFile, 0, SEEK_CUR);
	int size = lseek(historyFile, 0, SEEK_END);    //get length of file
	lseek(historyFile, currentPos, SEEK_SET);  //set position back to start
	char c[size+1];
	c[size+1] = '\0';
	int tracker = 0;
	int linesize = 0;
	if(read(historyFile, c, size) != 0){
		write(sockfd, c, size);
	}
	
	free(historyPath);
}

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
		printf("Performing commit command....\n");
		commit(token, sockfd);
    }else if (strcmp(token, "push") == 0){
    	printf("Performing push command....\n");
    	push(token, sockfd);
    }
    else if (strcmp(token, "rollback") == 0){
    	printf("Performing rollback command....\n");
    	rollback(token, sockfd);
    }
    else if (strcmp(token, "history") == 0){
    	printf("Performing history command....\n");
    	history(token, sockfd);
    }
    else{
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
