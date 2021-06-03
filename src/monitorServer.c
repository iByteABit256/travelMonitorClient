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
#include <time.h>
#include <dirent.h>
#include <poll.h>
#include "../lib/lists/lists.h"
#include "../lib/bloomfilter/bloomfilter.h"
#include "../lib/hashtable/htInterface.h"
#include "../src/parser.h"
#include "../src/vaccineMonitor.h"

#define INITIAL_BUFFSIZE 100
#define MAX_CONNECTIONS 1


// Parses parameters of executable
Listptr parseParameters(int argc, char *argv[], int *port, int *numThreads, int *socketBufferSize, int *cyclicBufferSize, int *sizeOfBloom){
    
    // Parameter indices
    int portIndex = -1;
    int sockBuff = -1;
    int cyclicBuff = -1;
    int bloom = -1;
    int threadnum = -1;

    int isOption[argc];
    memset(isOption, 0, sizeof(int)*argc);

    if(argc > 1){
        for(int i = 1; i < argc; i++){
            isOption[i] = 1;

            if(strcmp(argv[i],"-p") == 0){
                portIndex = i;
            }else if(strcmp(argv[i],"-b") == 0){
                sockBuff = i;
            }else if(strcmp(argv[i],"-c") == 0){
                cyclicBuff = i;
            }else if(strcmp(argv[i],"-s") == 0){
                bloom = i;
            }else if(strcmp(argv[i],"-t") == 0){
                threadnum = i;
            }else{
                isOption[i] = 0;
            }
        }
    }

    // Find last "-x [xVal]" option in order to find beggining of file paths
    int lastOption = -1;
    for(int i = argc; i >= 0; i--){
        if(isOption[i]){
            lastOption = i;
            break;
        }
    }

    if(portIndex == -1 || sockBuff == -1 || cyclicBuff == - 1 || bloom == -1 || threadnum == -1){
        fprintf(stderr, "ERROR: INCORRECT ARGUMENTS\n");
        exit(1);
    }

    if(portIndex+1 < argc){
        *port = atoi(argv[portIndex+1]);
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
        *sizeOfBloom = atoi(argv[bloom+1]);
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

    if(lastOption != -1 && lastOption+2 < argc){
        Listptr paths = ListCreate();

        for(int i = lastOption+2; i < argc; i++){
            ListInsertLast(paths, argv[i]);
        }
        return paths;
    }else{
        return NULL;
    }
}

// Frees up remaining memory after exiting
void freeMemory(HTHash countries, HTHash viruses, HTHash citizenRecords){
    for(int i = 0; i < countries->curSize; i++){
		for(Listptr l = countries->ht[i]->next; l != l->tail; l = l->next){
            Country country = ((HTEntry)(l->value))->item;
            free(country->name);
            free(country);
        }
    }

    for(int i = 0; i < citizenRecords->curSize; i++){
		for(Listptr l = citizenRecords->ht[i]->next; l != l->tail; l = l->next){
            Person per = ((HTEntry)(l->value))->item;
            free(per->citizenID);
            free(per->firstName);
            free(per->lastName);
            free(per);
        }
    }

    for(int i = 0; i < viruses->curSize; i++){
		for(Listptr l = viruses->ht[i]->next; l != l->tail; l = l->next){
            Virus vir = ((HTEntry)(l->value))->item;
            free(vir->name);
            bloomDestroy(vir->vaccinated_bloom);
            
            Skiplist skip = vir->vaccinated_persons;

            for(skipNode snode = skip->dummy->forward[0];
                snode != NULL; snode = snode->forward[0]){

                VaccRecord rec = snode->item;
                free(rec->date);
                free(rec);
            }

            skip = vir->not_vaccinated_persons;

            for(skipNode snode = skip->dummy->forward[0];
                snode != NULL; snode = snode->forward[0]){

                VaccRecord rec = snode->item;
                free(rec->date);
                free(rec);
            }

            skipDestroy(vir->vaccinated_persons);
            skipDestroy(vir->not_vaccinated_persons);

            free(vir->vaccinated_bloom);
            free(vir->vaccinated_persons);
            free(vir->not_vaccinated_persons);

            free(vir);
        }
    }

    HTDestroy(countries);
    HTDestroy(citizenRecords);
    HTDestroy(viruses);
}
  
int main(int argc, char *argv[]){

    char *logPath = "./logs/log_file.";

    int port;
    int numThreads;
    int sizeOfBloom;
    int socketBufferSize;
    int cyclicBufferSize;
    Listptr filepaths = NULL;

    filepaths = parseParameters(argc, argv, &port, &numThreads, &socketBufferSize, &cyclicBufferSize, &sizeOfBloom);

    printf("PID: %d\n\
            port: %d\n\
            numThreads: %d\n\
            sizeofbloom: %d\n\
            socketBufferSize: %d\n\
            cyclicBufferSize: %d\n\
            filepaths: %p\n", getpid(), port, numThreads, sizeOfBloom, socketBufferSize, cyclicBufferSize, filepaths);

    
    int sockfd;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;

    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);

    struct hostent *ent = gethostbyname(hostname);
    struct in_addr *locIP = (struct in_addr *)ent->h_addr_list[0];

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = locIP->s_addr;

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr))<0){
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, MAX_CONNECTIONS) < 0){
        perror("listen");
        exit(EXIT_FAILURE);
    }

    int new_socket;
    socklen_t addr_length = sizeof(addr);
    
    //while(1){
        // if((new_socket = accept(sockfd, (struct sockaddr *)&addr, &addr_length) < 0)){
        //     //continue;
        //     perror("accept");
        //     exit(EXIT_FAILURE);
        // }
        do{
            new_socket = accept(sockfd, (struct sockaddr *)&addr, &addr_length);
        }while(new_socket < 0);

        char buff[socketBufferSize];
        memset(buff, 0, socketBufferSize);
        strcpy(buff, "Hello!\n");

        write(new_socket, buff, socketBufferSize);

        printf("Wrote %s\n", buff);

        //close(new_socket);

        //break;
    //}

    // Listptr countryPaths = ListCreate();

    // fd = open(pipename, O_RDONLY);
    // fd2 = open(pipename2, O_WRONLY);

    // struct pollfd *pfds;
    // int num_open_fds, nfds;

    // num_open_fds = nfds = 1;

    // pfds = calloc(nfds, sizeof(struct pollfd));    
    // if(pfds == NULL){
    //     perror("calloc error\n");
    //     exit(1);
    // }

    // pfds[0].fd = fd;
    // pfds[0].events = POLLIN;

    // // Read buffersize, bloomsize and subdirectory paths from parent

    // int msgNum = 0;
    // while(num_open_fds > 0){
    //     int ready = poll(pfds, nfds, -1);

    //     if(ready == -1){
    //         perror("poll error\n");
    //         exit(1);
    //     }

    //     if(pfds[0].revents != 0){
    //         if(pfds[0].revents & POLLIN){

    //             int bytes_read = read(fd, buff, buffsize);

    //             if(bytes_read == -1){
    //                 perror("Error in read");
    //                 exit(1);
    //             }else if(bytes_read == 0){
    //                 continue;
    //             }

    //             if(msgNum == 0){
    //                 // First message

    //                 buffsize = atoi(buff);
    //             }else if(msgNum == 1){
    //                 // Second message

    //                 bloomsize = atoi(buff);
    //             }else{
    //                 // Subdirectory path

    //                 char *country = malloc(strlen(buff)+1);
    //                 strcpy(country, buff);
    //                 ListInsertLast(countryPaths, country);
    //             }

    //             msgNum++;
    //         }else{
    //             if(close(pfds[0].fd)){
    //                 perror("close error\n");
    //                 exit(1);
    //             }
    //             pfds[0].fd = -1; //Ignore events on next call
    //             num_open_fds--;
    //         }
    //     }
    // }

    // DIR *subdir;
    // struct dirent *direntp;

    // // Traverse subdirectories and save file paths in list

    // Listptr filepaths = ListCreate();

    // for(Listptr l = countryPaths->head->next; l != l->tail; l = l->next){
    //     char *subdirName = l->value;

    //     if((subdir = opendir(subdirName)) == NULL){
    //         fprintf(stderr, "Could not open %s\n", subdirName);
    //     }else{     
    //         while((direntp = readdir(subdir)) != NULL){
    //             if(strcmp(direntp->d_name, ".") && strcmp(direntp->d_name, "..")){
    //                 char *temp = malloc(buffsize*10);
    //                 strcpy(temp, subdirName);
    //                 strcat(temp, "/");
    //                 strcat(temp, direntp->d_name);
    //                 ListInsertLast(filepaths, temp);
    //             }
    //         }
    //         closedir(subdir);
    //     }
    //     free(subdirName);
    // }

    // ListDestroy(countryPaths);

    // HTHash viruses = HTCreate();
    // HTHash persons = HTCreate();
    // HTHash countries = HTCreate();
    
    // // Parse files

    // for(Listptr l = filepaths->head->next; l != l->tail; l = l->next){
    //     char *filepath = l->value;

    //     parseInputFile(filepath, bloomsize, persons, countries, viruses);

    //     free(filepath);
    // }

    // ListDestroy(filepaths);

    // // Send bloomfilters to parent
    //     // -First message is virus name
    //     // -Following messages are bloomfilter parts

    // int count = 1;
    // for(int i = 0; i < viruses->curSize; i++){
	// 	for(Listptr l = viruses->ht[i]->next; l != l->tail; l = l->next){
    //         HTEntry ht = l->value;
    //         Virus v = ht->item;

	// 		char *buff = malloc(sizeof(char)*buffsize);
    //         memset(buff, 0, sizeof(char)*buffsize);
    //         strcpy(buff, v->name);

    //         write(fd2, buff, buffsize);
    //         for(int i = 0; i <= bloomsize/buffsize; i++){
    //             if(i == bloomsize/buffsize){
    //                 // Remaining part of bloomfilter

    //                 memcpy(buff, v->vaccinated_bloom->bloom+i*buffsize, bloomsize%buffsize);
    //             }else{
    //                 memcpy(buff, v->vaccinated_bloom->bloom+i*buffsize, buffsize);
    //             }
    //             write(fd2, buff, buffsize);
    //         }
    //         free(buff);
    //         count++;
	// 	}
	// }

    // if(close(fd2)){
    //     perror("close error\n");
    //     exit(1);
    // }

    // fd = open(pipename, O_RDONLY);

    // int totalRequests = 0;
    // int accepted = 0;
    // int rejected = 0;

    // while(1){
    //     char buff[buffsize];
    //     int bytes_read = read(fd, buff, buffsize);

    //     if(bytes_read > 0){

    //         if(!strcmp(buff, "travelRequest")){
    //             totalRequests++;

    //             bytes_read = read(fd, buff, buffsize);

    //             char *id = malloc(strlen(buff)+1);
    //             strcpy(id, buff);

    //             bytes_read = read(fd, buff, buffsize);

    //             char *virName = malloc(strlen(buff)+1);
    //             strcpy(virName, buff);

    //             Virus v = HTGetItem(viruses, virName);

    //             VaccRecord rec = skipGet(v->vaccinated_persons, id);

    //             if(rec){
    //                 accepted++;
    //                 close(fd);
    //                 fd2 = open(pipename2, O_WRONLY);
    //                 strcpy(buff, "YES");
    //                 write(fd2, buff, buffsize);
    //                 strcpy(buff, "");
    //                 sprintf(buff, "%d", rec->date->day);
    //                 write(fd2, buff, buffsize);
    //                 strcpy(buff, "");
    //                 sprintf(buff, "%d", rec->date->month);
    //                 write(fd2, buff, buffsize);
    //                 strcpy(buff, "");
    //                 sprintf(buff, "%d", rec->date->year);
    //                 write(fd2, buff, buffsize);
    //                 close(fd2);
    //                 fd = open(pipename, O_RDONLY);
    //             }else{
    //                 rejected++;
    //                 close(fd);
    //                 fd2 = open(pipename2, O_WRONLY);
    //                 strcpy(buff, "NO");
    //                 write(fd2, buff, buffsize);
    //                 close(fd2);
    //                 fd = open(pipename, O_RDONLY);
    //             }

    //             free(id);
    //             free(virName);
    //         }
    //         if(!strcmp(buff, "exit")){
    //             break;
    //         }
    //     }
    // }

    // // Close file descriptors

    // close(fd);
    // close(fd2);

    // // Memory freeing

    // freeMemory(countries, viruses, persons);

    // free(buff);
    // free(pfds);

    exit(0);
}
