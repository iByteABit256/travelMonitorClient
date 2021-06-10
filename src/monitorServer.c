#include <linux/limits.h>
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
#include <pthread.h>
#include "../lib/lists/lists.h"
#include "../lib/bloomfilter/bloomfilter.h"
#include "../lib/hashtable/htInterface.h"
#include "../src/parser.h"
#include "../src/vaccineMonitor.h"
#include "../lib/buffer/buffer.h"

#define INITIAL_BUFFSIZE 100
#define MAX_CONNECTIONS 3

pthread_mutex_t lock;
int file_count;

//TODO: cyclic buffer bytes -> number of paths in cyclic buffer


// Thread function for file parsing
void *parserThreadFunc(void *vargp){
    Database db = vargp;

    while(1){

        //printf("About to enter CS\n");

        pthread_mutex_lock(&lock);

        //printf("file count is %d\n", file_count);

        if(file_count <= 0){
            pthread_mutex_unlock(&lock);
            break;
        }

        if(bufferEmpty(db->cyclicBuff, db->cyclicBufferSize)){
            pthread_mutex_unlock(&lock);
            continue;
        }

        char *filepath = buffGetLast(db->cyclicBuff, db->cyclicBufferSize);
        //printf("parsing %s\n", filepath);
        parseInputFile(filepath, db->sizeOfBloom, db->persons, db->countries, db->viruses);
        //printf("parsed %s\n", filepath);

        free(filepath);

        file_count--;

        //printf("About to exit CS\n");

        pthread_mutex_unlock(&lock);
    }

    //printf("Thread done\n");

    return db;
}

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
    for(int i = argc-1; i >= 0; i--){
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
    Listptr countryPaths = NULL;

    countryPaths = parseParameters(argc, argv, &port, &numThreads, &socketBufferSize, &cyclicBufferSize, &sizeOfBloom);

    /*printf("PID: %d\n\
            port: %d\n\
            numThreads: %d\n\
            sizeofbloom: %d\n\
            socketBufferSize: %d\n\
            cyclicBufferSize: %d\n\
            filepaths: %p\n", getpid(), port, numThreads, sizeOfBloom, socketBufferSize, cyclicBufferSize, countryPaths);
    */

    int sockfd, new_socket;
    struct sockaddr_in addr;
    //int opt = 1;
    int addrlen = sizeof(addr);
    char buffer[socketBufferSize];
    memset(buffer, 0, socketBufferSize);
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);

    struct hostent *ent = gethostbyname(hostname);
    struct in_addr *locIP = (struct in_addr *)ent->h_addr_list[0];

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = locIP->s_addr;
       
    if(bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if(listen(sockfd, MAX_CONNECTIONS) < 0){
        perror("listen");
        exit(EXIT_FAILURE);
    }

    if((new_socket = accept(sockfd, (struct sockaddr *)&addr, (socklen_t*)&addrlen)) < 0){
        perror("accept");
        exit(EXIT_FAILURE);
    }

    DIR *subdir;
    struct dirent *direntp;

    // Traverse subdirectories and save file paths in list

    Listptr filepaths = ListCreate();

    for(Listptr l = countryPaths->head->next; l != l->tail; l = l->next){
        char *subdirName = l->value;

        if((subdir = opendir(subdirName)) == NULL){
            fprintf(stderr, "Could not open %s\n", subdirName);
        }else{     
            while((direntp = readdir(subdir)) != NULL){
                if(strcmp(direntp->d_name, ".") && strcmp(direntp->d_name, "..")){
                    char *temp = malloc(socketBufferSize*100);
                    strcpy(temp, subdirName);
                    strcat(temp, "/");
                    strcat(temp, direntp->d_name);
                    ListInsertLast(filepaths, temp);

                    //free(l->value);
                }
            }
            closedir(subdir);
        }
    }

    ListDestroy(countryPaths);

    Database db = malloc(sizeof(struct databasestr));

    db->viruses = HTCreate();
    db->persons = HTCreate();
    db->countries = HTCreate();

    char *cyclicBuff = malloc(cyclicBufferSize);
    memset(cyclicBuff, 0, cyclicBufferSize);
    
    db->cyclicBuff = cyclicBuff;
    db->cyclicBufferSize = cyclicBufferSize;
    db->sizeOfBloom = sizeOfBloom;

    file_count = ListSize(filepaths);

    // Parse files

    if(pthread_mutex_init(&lock, NULL) != 0){
        perror("mutex init");
        exit(1);
    }

    pthread_t tid[numThreads];

    //printf("Mutex initialized\n");

    for(int i = 0; i < numThreads; i++){
        if(pthread_create(&tid[i], NULL, parserThreadFunc, db) != 0){
            perror("pthread create");
            exit(1);
        }
    }

    //printf("Threads created\n");

    while(ListSize(filepaths) > 0){
        char *filepath = ListGetLast(filepaths)->value;
        if(strFits(cyclicBuff, cyclicBufferSize, filepath)){
            //printf("adding %s to buff\n", filepath);
            buffInsert(cyclicBuff, cyclicBufferSize, filepath);
            free(filepath);
            ListDeleteLast(filepaths);
        }
    }

    //printf("All files added\n");

    ListDestroy(filepaths);

    for(int i = 0; i < numThreads; i++){
        if(pthread_join(tid[i], NULL) != 0){
            perror("pthread join");
            exit(1);
        }
    }

    if(pthread_mutex_destroy(&lock) != 0){
        perror("mutex destroy");
        exit(1);
    }

    free(cyclicBuff);

    //printf("Threads joined and mutex detroyed\n");

    //popStatusByAge(HTGetItem(db->viruses, "COVID-19"), NULL, NULL, db->countries, NULL);

    // Send bloomfilters to parent
        // -First message is virus name
        // -Following messages are bloomfilter parts

    int count = 1;
    for(int i = 0; i < db->viruses->curSize; i++){
		for(Listptr l = db->viruses->ht[i]->next; l != l->tail; l = l->next){
            HTEntry ht = l->value;
            Virus v = ht->item;

			char *buff = malloc(sizeof(char)*socketBufferSize);
            memset(buff, 0, sizeof(char)*socketBufferSize);
            strcpy(buff, v->name);

            write(new_socket, buff, socketBufferSize);
            for(int i = 0; i <= sizeOfBloom/socketBufferSize; i++){
                if(i == sizeOfBloom/socketBufferSize){
                    // Remaining part of bloomfilter

                    memcpy(buff, v->vaccinated_bloom->bloom+i*socketBufferSize, sizeOfBloom%socketBufferSize);
                }else{
                    memcpy(buff, v->vaccinated_bloom->bloom+i*socketBufferSize, socketBufferSize);
                }
                write(new_socket, buff, socketBufferSize);
            }
            free(buff);
            count++;
		}
	}

    char buff[socketBufferSize];
    memset(buff, 0, socketBufferSize);
    strcpy(buff, "done");

    send(new_socket, buff, socketBufferSize, 0);

    int totalRequests = 0;
    int accepted = 0;
    int rejected = 0;

    while(1){
        char buff[socketBufferSize];
        int bytes_read = read(new_socket, buff, socketBufferSize);

        if(bytes_read > 0){

            if(!strcmp(buff, "travelRequest")){
                totalRequests++;

                bytes_read = read(new_socket, buff, socketBufferSize);

                char *id = malloc(strlen(buff)+1);
                strcpy(id, buff);

                bytes_read = read(new_socket, buff, socketBufferSize);

                char *virName = malloc(strlen(buff)+1);
                strcpy(virName, buff);

                Virus v = HTGetItem(db->viruses, virName);

                VaccRecord rec = skipGet(v->vaccinated_persons, id);

                if(rec){
                    accepted++;
                    strcpy(buff, "YES");
                    write(new_socket, buff, socketBufferSize);
                    strcpy(buff, "");
                    sprintf(buff, "%d", rec->date->day);
                    write(new_socket, buff, socketBufferSize);
                    strcpy(buff, "");
                    sprintf(buff, "%d", rec->date->month);
                    write(new_socket, buff, socketBufferSize);
                    strcpy(buff, "");
                    sprintf(buff, "%d", rec->date->year);
                    write(new_socket, buff, socketBufferSize);
                }else{
                    rejected++;
                    strcpy(buff, "NO");
                    write(new_socket, buff, socketBufferSize);
                }

                free(id);
                free(virName);
            }
            if(!strcmp(buff, "exit")){
                break;
            }
        }
    }

    // Close file descriptors

    close(new_socket);

    // Memory freeing

    freeMemory(db->countries, db->viruses, db->persons);
    free(db);

    exit(0);
}
