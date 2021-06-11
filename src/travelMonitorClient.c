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

    HTHash viruses = HTCreate();

    int msgNum[numMonitors];
    char *curVirus[numMonitors];
    BloomFilter tempBloom[numMonitors];

    for(int i = 0; i < numMonitors; i++){
        msgNum[i] = 0;
        tempBloom[i] = bloomInitialize(sizeOfBloom);
    }

    while(num_open_fds > 0){
        int ready = poll(pfds, nfds, -1);

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
                        exit(1);
                    }else if(bytes_read == 0){
                        printf("continuing...\n");
                        continue;
                    }

                    if(!strcmp(buff, "done")){

                        pfds[i].fd = -2; //Ignore events on next call
                        num_open_fds--; 
                    }

                    // First message is virus name
                    if(msgNum[i] == 0){

                        char *virName = malloc((strlen(buff)+1)*sizeof(char));
                        strcpy(virName, buff);

                        curVirus[i] = malloc(strlen(virName)+1);
                        strcpy(curVirus[i], virName);

                        if(!HTExists(viruses, virName)){
                            Virus vir = newVirus(virName, sizeOfBloom, 9, 0.5);
                            HTInsert(viruses, vir->name, vir);
                        }

                        free(virName);
                    
                    }else if(msgNum[i]-1 < sizeOfBloom/socketBufferSize){   
                        // Bloomfilter parts copied to temporary bloomfilter

                        memcpy(tempBloom[i]->bloom+(msgNum[i]-1)*socketBufferSize, buff, socketBufferSize);
                    }else{
                        // Temporary bloomfilter acts as a mask on the actual bloomfilter

                        Virus vir = HTGetItem(viruses, curVirus[i]);
                        BloomFilter bF = vir->vaccinated_bloom;
                        bloomOR(bF, tempBloom[i]);

                        msgNum[i] = 0;
                        free(curVirus[i]);
                        curVirus[i] = NULL;
                        bloomDestroy(tempBloom[i]);
                        free(tempBloom[i]);
                        tempBloom[i] = bloomInitialize(sizeOfBloom);

                        continue;
                    }

                    msgNum[i]++;

                }else{
                    pfds[i].fd = -2; //Ignore events on next call
                    num_open_fds--;
                }
            }
        }
    }

    for(int i = 0; i < numMonitors; i++){
        for(int j = 0; j < mon_argc[i]; j++){
            if(j != 0 && j != 1 && j != 3 && j != 5 && j != 7 && j != 9){
                free(mon_argv[i][j]);
            }
        }
        free(mon_argv[i]);
    }

    char *inbuf = malloc(MAX_LINE*sizeof(char));
    memset(inbuf, 0, MAX_LINE);

    // Input loop

    int totalRequested = 0;
    int accepted = 0;
    int rejected = 0;

    printf("TRAVEL MONITOR\n-----------------\n");

    while(1){
        // Read input
        if(fgets(inbuf, MAX_LINE, stdin) != NULL){

            char *token = strtok(inbuf, " \n"); 

            if(token == NULL){
                fprintf(stderr, "ERROR: INCORRECT SYNTAX\n\n");
                continue;
            }

            if(!strcmp(token, "/travelRequest")){
                totalRequested++;

                char *id;
                char *date_str;
                Date date;
                char *countryFrom;
                char *countryTo;
                char *virName;

                id = strtok(NULL, " \n");

                if(id == NULL){
                    fprintf(stderr, "ERROR: INCORRECT SYNTAX\n\n");
                    continue;
                }

                date_str = strtok(NULL, " \n");

                if(date_str == NULL){
                    fprintf(stderr, "ERROR: INCORRECT SYNTAX\n\n");
                    continue;
                }        

                countryFrom = strtok(NULL, " \n");

                if(countryFrom == NULL){
                    fprintf(stderr, "ERROR: INCORRECT SYNTAX\n\n");
                    continue;
                }

                countryTo = strtok(NULL, " \n");

                if(countryTo == NULL){
                    fprintf(stderr, "ERROR: INCORRECT SYNTAX\n\n");
                    continue;
                }

                virName = strtok(NULL, " \n");

                if(virName == NULL){
                    fprintf(stderr, "ERROR: INCORRECT SYNTAX\n\n");
                    continue;
                }

                date = malloc(sizeof(struct datestr));

                char *datetok1 = strtok(date_str, "-\n");
                char *datetok2 = strtok(NULL, "-\n");
                char *datetok3 = strtok(NULL, "-\n");

                // check if date is valid
                if(datetok1 != NULL && datetok2 != NULL && datetok3 != NULL){
                    date->day = atoi(datetok1); 
                    date->month = atoi(datetok2); 
                    date->year = atoi(datetok3);
                }else{
                    fprintf(stderr, "ERROR: INCORRECT SYNTAX\n\n");
                    free(date);
                    continue;
                }

                Virus v = HTGetItem(viruses, virName);
                if(v == NULL){
                    printf("ERROR: NO DATA ABOUT GIVEN VIRUS\n\n");
                    free(date);
                    continue;
                }

                if(bloomExists(v->vaccinated_bloom, id)){
                    int mon = getMonitorNum(numMonitors, countries, countryFrom);

                    char buff[socketBufferSize];
                    memset(buff, 0, socketBufferSize);

                    strcpy(buff, "travelRequest");
                    write(sockfd[mon], buff, socketBufferSize);

                    strcpy(buff, id);
                    write(sockfd[mon], buff, socketBufferSize);

                    strcpy(buff, virName);
                    write(sockfd[mon], buff, socketBufferSize);

                    strcpy(buff, countryTo);
                    write(sockfd[mon], buff, socketBufferSize);

                    sprintf(buff, "%d", date->day);
                    write(sockfd[mon], buff, socketBufferSize);

                    sprintf(buff, "%d", date->month);
                    write(sockfd[mon], buff, socketBufferSize);

                    sprintf(buff, "%d", date->year);
                    write(sockfd[mon], buff, socketBufferSize);

                    read(sockfd[mon], buff, socketBufferSize);

                    if(!strcmp(buff, "YES")){
                        Date vacc_date = malloc(sizeof(struct datestr));
                        read(sockfd[mon], buff, socketBufferSize);
                        vacc_date->day = atoi(buff);
                        read(sockfd[mon], buff, socketBufferSize);
                        vacc_date->month = atoi(buff);
                        read(sockfd[mon], buff, socketBufferSize);
                        vacc_date->year = atoi(buff);

                        if(getDiffDate(date, vacc_date)/30 >= 6){
                            printf("REQUEST REJECTED – YOU WILL NEED ANOTHER VACCINATION BEFORE TRAVEL DATE\n\n");
                            rejected++;
                        }else{
                            printf("REQUEST ACCEPTED – HAPPY TRAVELS\n\n");
                            accepted++;
                        }

                        free(vacc_date);

                    }else if(!strcmp(buff, "NO")){
                        printf("REQUEST REJECTED – YOU ARE NOT VACCINATED\n\n");
                        rejected++;
                    }

                }else{
                    printf("REQUEST REJECTED – YOU ARE NOT VACCINATED\n\n");
                    rejected++;
                }
                free(date);
            }

            if(!strcmp(token, "/searchVaccinationStatus")){
                char *id = strtok(NULL, " \n");

                if(id == NULL){
                    fprintf(stderr, "ERROR: INCORRECT SYNTAX\n\n");
                    continue;
                }

                for(int i = 0; i < numMonitors; i++){
                    char buff[socketBufferSize];
                    memset(buff, 0, socketBufferSize);
                    strcpy(buff, "searchVaccinationStatus");

                    write(sockfd[i], buff, socketBufferSize);

                    strcpy(buff, id);
                    write(sockfd[i], buff, socketBufferSize);

                    // while(read(sockfd[i], buff, socketBufferSize) == 0){
                    //     continue;
                    // }
                    read(sockfd[i], buff, socketBufferSize);

                    if(!strcmp(buff, "found")){
                        while(1){
                            // while(read(sockfd[i], buff, socketBufferSize) == 0){
                            //     continue;
                            // }
                            read(sockfd[i], buff, socketBufferSize);
                            if(!strcmp(buff, "done")){
                                printf("\n");
                                break;
                            }
                            printf("%s", buff);
                        }
                    }else if(!strcmp(buff, "not found")){
                        continue;
                    }
                }
            }

            if(!strcmp(token, "/travelStats")){
                char *virName;
                char *date1_str;
                char *date2_str;
                char *country;
                Date date1, date2;

                virName = strtok(NULL, " \n");

                if(virName == NULL){
                    fprintf(stderr, "ERROR: INCORRECT SYNTAX\n\n");
                    continue;
                }

                date1_str = strtok(NULL, " \n");

                if(date1_str == NULL){
                    fprintf(stderr, "ERROR: INCORRECT SYNTAX\n\n");
                    continue;
                }

                date2_str = strtok(NULL, " \n");

                if(date1_str == NULL){
                    fprintf(stderr, "ERROR: INCORRECT SYNTAX\n\n");
                    continue;
                }

                country = strtok(NULL, " \n");

                date1 = malloc(sizeof(struct datestr));

                char *datetok1 = strtok(date1_str, "-\n");
                char *datetok2 = strtok(NULL, "-\n");
                char *datetok3 = strtok(NULL, "-\n");

                // check if date is valid
                if(datetok1 != NULL && datetok2 != NULL && datetok3 != NULL){
                    date1->day = atoi(datetok1); 
                    date1->month = atoi(datetok2); 
                    date1->year = atoi(datetok3);
                }else{
                    fprintf(stderr, "ERROR: INCORRECT SYNTAX\n\n");
                    free(date1);
                    continue;
                }

                date2 = malloc(sizeof(struct datestr));

                datetok1 = strtok(date2_str, "-\n");
                datetok2 = strtok(NULL, "-\n");
                datetok3 = strtok(NULL, "-\n");

                // check if date is valid
                if(datetok1 != NULL && datetok2 != NULL && datetok3 != NULL){
                    date2->day = atoi(datetok1); 
                    date2->month = atoi(datetok2); 
                    date2->year = atoi(datetok3);
                }else{
                    fprintf(stderr, "ERROR: INCORRECT SYNTAX\n\n");
                    free(date2);
                    continue;
                }

                int totalRequested = 0;
                int accepted = 0;
                int rejected = 0;

                char buff[socketBufferSize];
                memset(buff, 0, socketBufferSize);

                for(int i = 0; i < numMonitors; i++){
                    strcpy(buff, "travelStats");
                    write(sockfd[i], buff, socketBufferSize);

                    strcpy(buff, virName);
                    write(sockfd[i], buff, socketBufferSize);

                    sprintf(buff, "%d", date1->day);
                    write(sockfd[i], buff, socketBufferSize);

                    sprintf(buff, "%d", date1->month);
                    write(sockfd[i], buff, socketBufferSize);

                    sprintf(buff, "%d", date1->year);
                    write(sockfd[i], buff, socketBufferSize);

                    sprintf(buff, "%d", date2->day);
                    write(sockfd[i], buff, socketBufferSize);

                    sprintf(buff, "%d", date2->month);
                    write(sockfd[i], buff, socketBufferSize);

                    sprintf(buff, "%d", date2->year);
                    write(sockfd[i], buff, socketBufferSize);

                    if(country == NULL){
                        strcpy(buff, "NO COUNTRY");
                    }else{
                        strcpy(buff, country);
                    }
                    write(sockfd[i], buff, socketBufferSize);

                    read(sockfd[i], buff, socketBufferSize);
                    totalRequested += atoi(buff);

                    read(sockfd[i], buff, socketBufferSize);
                    accepted += atoi(buff);

                    read(sockfd[i], buff, socketBufferSize);
                    rejected += atoi(buff);
                }

                printf("TOTAL REQUESTS: %d\nACCEPTED: %d\nREJECTED: %d\n\n", totalRequested, accepted, rejected);
                free(date1);
                free(date2);
            }

            if(!strcmp(token, "/exit")){
                char buff[socketBufferSize];
                memset(buff, 0, socketBufferSize);
                strcpy(buff, "exit");

                for(int i = 0; i < numMonitors; i++){
                    write(sockfd[i], "exit", socketBufferSize);
                }

                break;
            }
        }
    }

    // Memory freeing

    char extension[10];
    sprintf(extension, "%d", getpid());

    char *logFile = malloc(strlen(logPath)+11);
    strcpy(logFile, logPath);
    strcat(logFile, extension);

    FILE *log = fopen(logFile, "a");

    free(logFile);

    for(int i = 0; i < numMonitors; i++){
        HTHash ht = countries[i];
        for(int i = 0; i < ht->curSize; i++){
            for(Listptr l = ht->ht[i]->next; l != l->tail; l = l->next){
                HTEntry ht = l->value;
                fprintf(log, "%s\n", (char *)ht->item); 
            }
        }
    }

    fprintf(log, "TOTAL TRAVEL REQUESTS %d\n", totalRequested);
    fprintf(log, "ACCEPTED %d\n", accepted);
    fprintf(log, "REJECTED %d\n", rejected);

    fclose(log);

    free(inbuf);

    for(int i = 0; i < viruses->curSize; i++){
		for(Listptr l = viruses->ht[i]->next; l != l->tail; l = l->next){
            HTEntry ht = l->value;
            Virus v = ht->item;
            destroyVirus(v);
            free(v);
        }
    }

    HTDestroy(viruses);
    free(pfds);
    
    for(int i = 0; i < numMonitors; i++){
        HTHash ht = countries[i];
        for(int i = 0; i < ht->curSize; i++){
            for(Listptr l = ht->ht[i]->next; l != l->tail; l = l->next){
                HTEntry ht = l->value;
                free(ht->item);   
            }
        }
        HTDestroy(ht);
        bloomDestroy(tempBloom[i]);
        free(tempBloom[i]);
    }

    return 0;
}
