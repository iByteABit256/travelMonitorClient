#include <bits/types/siginfo_t.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <poll.h>
#include "../src/vaccineMonitor.h"
#include "../lib/lists/lists.h"
#include "../lib/bloomfilter/bloomfilter.h"
#include "../lib/hashtable/htInterface.h"

#define INITIAL_BUFFSIZE 100
#define MAX_LINE 100
#define S_PORT 4000


// String compare function wrapper
int compareStr(void *a, void *b){
    return strcmp((char *)a, (char *)b);
}

// Parses parameters of executable
void parseExecutableParameters(int argc, char *argv[], int *monitorNum, int *socketBufferSize, int *cyclicBufferSize, int *bloomSize, char **inDir, int *numThreads){
    
    // Parameter indices
    int monitor = -1;
    int sockBuff = -1;
    int cyclicBuff = -1;
    int bloom = -1;
    int dir = -1;
    int threadnum = -1;

    if(argc > 1){
        for(int i = 1; i < argc; i++){
            if(strcmp(argv[i],"-m") == 0){
                monitor = i;
            }else if(strcmp(argv[i],"-b") == 0){
                sockBuff = i;
            }else if(strcmp(argv[i],"-c") == 0){
                cyclicBuff = i;
            }else if(strcmp(argv[i],"-s") == 0){
                bloom = i;
            }else if(strcmp(argv[i],"-i") == 0){
                dir = i;
            }else if(strcmp(argv[i],"-t") == 0){
                threadnum = i;
            }
        }
    }

    if(monitor == -1 || sockBuff == -1 || cyclicBuff == - 1 || bloom == -1 || dir == -1 || threadnum == -1){
        fprintf(stderr, "ERROR: INCORRECT ARGUMENTS\n");
        exit(1);
    }

    if(monitor+1 < argc){
        *monitorNum = atoi(argv[monitor+1]);
    }else{
        fprintf(stderr, "ERROR: INCORRECT ARGUMENTS\n");
        exit(1);
    }

    if(sockBuff+1 < argc){
        *socketBufferSize = atoi(argv[sockBuff+1]);
    }else{
        fprintf(stderr, "ERROR: INCORRECT ARGUMENTS\n");
        exit(1);
    }

    if(cyclicBuff+1 < argc){
        *cyclicBufferSize = atoi(argv[cyclicBuff+1]);
    }else{
        fprintf(stderr, "ERROR: INCORRECT ARGUMENTS\n");
        exit(1);
    }

    if(bloom+1 < argc){
        *bloomSize = atoi(argv[bloom+1]);
    }else{
        fprintf(stderr, "ERROR: INCORRECT ARGUMENTS\n");
        exit(1);
    } 

    if(dir+1 < argc){
        *inDir = argv[dir+1];
    }else{
        fprintf(stderr, "ERROR: INCORRECT ARGUMENTS\n");
        exit(1);
    }

    if(threadnum+1 < argc){
        *numThreads = atoi(argv[threadnum+1]);
    }else{
        fprintf(stderr, "ERROR: INCORRECT ARGUMENTS\n");
        exit(1);
    }
}

int getDigits(int num){
    int i = 0;
    while((num = num/10)) i++;
    return i+1;
}

int getMonitorNum(int numMonitors, HTHash *countries, char *country){
    for(int i = 0; i < numMonitors; i++){
        if(HTExists(countries[i], country)){
            return i;
        }
    }

    return -1;
}

int main(int argc, char *argv[]){

    int numMonitors;
    int socketBufferSize;
    int cyclicBufferSize;
    int sizeOfBloom;
    char *input_dir = NULL;
    int numThreads;

    parseExecutableParameters(argc, argv, &numMonitors, &socketBufferSize, &cyclicBufferSize, &sizeOfBloom, &input_dir, &numThreads);

    if(socketBufferSize < 64){
        fprintf(stderr, "buff size must be at least 64 bytes\n");
        return 1;
    }
  
    // Logfile path
    char *logPath = "./logs/log_file.";

    int ports[numMonitors];
    for(int i = 0; i < numMonitors; i++){
        ports[i] = S_PORT+i;
    }

    //pid_t pids[numMonitors];
    
    char **mon_argv[numMonitors];
    int mon_argc[numMonitors];

    // Known parameters, file paths added later
    for(int i = 0; i < numMonitors; i++){
        mon_argc[i] = 11;
        mon_argv[i] = malloc(sizeof(char *)*11);
        
        mon_argv[i][0] = "./monitorServer";
        mon_argv[i][1] = "-p";
        mon_argv[i][2] = malloc((getDigits(ports[i])+1)*sizeof(char));
        sprintf(mon_argv[i][2], "%d", ports[i]);
        mon_argv[i][3] = "-t";
        mon_argv[i][4] = malloc((getDigits(numThreads)+1)*sizeof(char));
        sprintf(mon_argv[i][4], "%d", numThreads);
        mon_argv[i][5] = "-b";
        mon_argv[i][6] = malloc((getDigits(socketBufferSize)+1)*sizeof(char));
        sprintf(mon_argv[i][6], "%d", socketBufferSize);
        mon_argv[i][7] = "-c";
        mon_argv[i][8] = malloc((getDigits(cyclicBufferSize)+1)*sizeof(char));
        sprintf(mon_argv[i][8], "%d", cyclicBufferSize);
        mon_argv[i][9] = "-s";
        mon_argv[i][10] = malloc((getDigits(sizeOfBloom)+1)*sizeof(char));
        sprintf(mon_argv[i][10], "%d", sizeOfBloom);
    }

    DIR *inDir;
    struct dirent *direntp = NULL;

    Listptr subdirs = ListCreate();

    // Traverse input directory and add subdirectory names to list
    if((inDir = opendir(input_dir)) == NULL){
        fprintf(stderr, "Could not open %s\n", input_dir);
    }else{
        while((direntp = readdir(inDir)) != NULL){
            char *subdir = malloc(sizeof(char)*socketBufferSize);
            memset(subdir, 0, sizeof(char)*socketBufferSize);
            strcat(subdir, input_dir);
            strcat(subdir, direntp->d_name);
            ListInsertSorted(subdirs, subdir, &compareStr);
        }
        closedir(inDir);
    }

    char *current = malloc(strlen(input_dir)+2);
    strcpy(current, input_dir);
    strcat(current, ".");

    char *previous = malloc(strlen(input_dir)+3);
    strcpy(previous, input_dir);
    strcat(previous, "..");

    HTHash countries[numMonitors];
    for(int i = 0 ; i < numMonitors; i++){
        countries[i] = HTCreate();
    }

    // Pass subdirectories to children with round robin order

    int count = 0;
    for(Listptr l = subdirs->head->next; l != l->tail; l = l->next){
        char *filePath = malloc(strlen(l->value)+1);
        char *dirname = malloc(strlen(l->value)+1);
        strcpy(dirname, l->value);
        strcpy(filePath, l->value);


        if(!strcmp(dirname, current) || !strcmp(dirname, previous)){
            free(dirname);
            continue;
        }

        char *temp = dirname;
        char *prev = dirname;
        dirname = strtok(dirname, "/");

        while(dirname != NULL){
            prev = dirname;
            dirname = strtok(NULL, "/");
        }

        char *country = malloc(strlen(prev)+1);
        strcpy(country, prev);

        free(temp);

        int mon = count%numMonitors;
        HTInsert(countries[mon], country, country);
        mon_argv[mon] = realloc(mon_argv[mon], ++mon_argc[mon]*sizeof(char *));
        mon_argv[mon][mon_argc[mon]-1] = filePath;

        count++;
    }

    for(int i = 0; i < numMonitors; i++){
        mon_argv[i] = realloc(mon_argv[i], ++mon_argc[i]*sizeof(char *));
        mon_argv[i][mon_argc[i]-1] = NULL;
    }

    for(Listptr l = subdirs->head->next; l != l->tail; l = l->next){
        free(l->value);
    }
    ListDestroy(subdirs);
    free(current);
    free(previous);

    // Fork
    for(int i = 0; i < numMonitors; i++){
        pid_t pid = fork();

        if(pid == -1){
            // Error
            fprintf(stderr, "Error during fork\n");
            exit(1);
        }else if(pid == 0){
            // Forked process
            execv("./monitorServer", mon_argv[i]);
            exit(1);
        }else{
            //pids[i] = pid;
        }
    }

    int sockfd[numMonitors];
	struct sockaddr_in addr[numMonitors];

    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);

    struct hostent *ent = gethostbyname(hostname);
    struct in_addr *locIP = (struct in_addr *)ent->h_addr_list[0];

    for(int i = 0; i < numMonitors; i++){

    	if((sockfd[i] = socket(AF_INET, SOCK_STREAM, 0)) < 0){
     	    printf("\n Socket creation error \n");
    	    return -1;
   	 	}
   
    	addr[i].sin_family = AF_INET;
    	addr[i].sin_port = htons(ports[i]);
        addr[i].sin_addr.s_addr = locIP->s_addr;
   		
    	while(connect(sockfd[i], (struct sockaddr *)&addr[i], sizeof(addr[i])) < 0){}
	}

    struct pollfd *pfds;
    int num_open_fds, nfds;

    num_open_fds = nfds = numMonitors;

    pfds = calloc(nfds, sizeof(struct pollfd));    
    if(pfds == NULL){
        perror("calloc error");
        exit(1);
    }

    for(int i = 0; i < numMonitors; i++){
        pfds[i].fd = sockfd[i];
        pfds[i].events = POLLIN;
    }

    while(num_open_fds > 0){
        int ready = poll(pfds, nfds, -1);
        //printf("ready = %d\n", ready);

        if(ready == -1){
            perror("poll error\n");
            exit(1);
        }

        for(int i = 0; i < numMonitors; i++){
            if(pfds[i].revents != 0){
                if(pfds[i].revents & POLLIN){
                    char buff[socketBufferSize];
                    memset(buff, 0, socketBufferSize);

                    int bytes_read = recv(pfds[i].fd, buff, socketBufferSize, 0);

                    if(bytes_read == -1){
                        perror("Error in read");
                        //continue;
                        exit(1);
                    }else if(bytes_read == 0){
                        printf("continuing...\n");
                        continue;
                    }

                    printf("Parent - Read %s from fd %d\n", buff, pfds[i].fd);

                    if(!strcmp(buff, "Done!\n")){
                        strcpy(buff, "bye!\n");
                        printf("Parent - Writing %s to fd %d\n", buff, pfds[i].fd);
                        send(pfds[i].fd, buff, socketBufferSize, 0);

                        pfds[i].fd = -2; //Ignore events on next call
                        num_open_fds--; 
                    }

                }else{
                    // if(close(pfds[i].fd)){
                    //     perror("close error\n");
                    //     exit(1);
                    // }
                    pfds[i].fd = -2; //Ignore events on next call
                    num_open_fds--;
                }
            }
        }
    }

    return 0;
}
