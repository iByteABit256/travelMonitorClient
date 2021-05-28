#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vaccineMonitor.h"
#define MIN(a,b) a < b? a : b


// Deletes record and date, rest are dealt with in main
void deleteRecord(VaccRecord rec){
    free(rec->date);
    free(rec);
}

// Initialize new country
Country newCountry(char *name){
    Country country = malloc(sizeof(struct countrystr));
    memset(country, 0, sizeof(struct countrystr));
    country->name = name;

    return country;
}

// Initialize new virus
Virus newVirus(char *name, int bloomSize, int maxlvl, float tossChance){
    Virus vir = malloc(sizeof(struct virusstr));
    vir->name = malloc((strlen(name)+1)*sizeof(char)); 
    strcpy(vir->name, name);
    vir->vaccinated_bloom = bloomInitialize(bloomSize);
    vir->vaccinated_persons = newSkiplist(maxlvl, tossChance);
    vir->not_vaccinated_persons = newSkiplist(maxlvl, tossChance);

    return vir;
}

// Destroy virus
void destroyVirus(Virus v){
    free(v->name);
    bloomDestroy(v->vaccinated_bloom);
    free(v->vaccinated_bloom);
    skipDestroy(v->vaccinated_persons);
    free(v->vaccinated_persons);
    skipDestroy(v->not_vaccinated_persons);
    free(v->not_vaccinated_persons);
}

// Increments correct age group population
void incrementAgePopulation(Country country, int age){
    country->agePopulation[MIN(age/20, 3)] += 1;
}

// Increments correct age group counter
void incrementAgeCounter(Country country, int age){
    country->ageCounter[MIN(age/20, 3)] += 1;
}

// Insert new citizen record related to virus
void insertCitizenRecord(VaccRecord rec, Virus vir){
    if(rec->date == NULL){
        // Citizen is not vaccinated

        // Confirm that citizen is not vaccinated, print error otherwise
        if(!bloomExists(vir->vaccinated_bloom, rec->per->citizenID)){
            skipInsert(vir->not_vaccinated_persons, rec->per->citizenID, rec);
        }else{
            VaccRecord found = skipGet(vir->vaccinated_persons, rec->per->citizenID);
            if(found){
                printf("ERROR: CITIZEN %s ALREADY VACCINATED\n\n", rec->per->citizenID);
                deleteRecord(rec);
            }else{
                skipInsert(vir->not_vaccinated_persons, rec->per->citizenID, rec);
            }
        }
        //printf("Inserted %s to not-vaccinated list\n", rec->per->lastName);
    }else{
        // Citizen is vaccinated
        
        VaccRecord found = skipGet(vir->vaccinated_persons, rec->per->citizenID);
        if(found){
            // Vaccination already recorded
            Person p = found->per;
            Date d = found->date;
            printf("ERROR: CITIZEN %s ALREADY VACCINATED ON %02d-%02d-%04d\n\n",
                    p->citizenID, d->day, d->month, d->year);
            deleteRecord(rec);
        }else{
            // Citizen is inserted
            skipInsert(vir->vaccinated_persons, rec->per->citizenID, rec);
            bloomInsert(vir->vaccinated_bloom, rec->per->citizenID); 
            //printf("Inserted %s to vaccinated list\n", rec->per->lastName);
        }
    }
}

// check if citizen exists in virus bloom filter
int vaccineStatusBloom(char *citizenID, Virus vir){
    // virus doesn't exist
    if(vir == NULL){
        fprintf(stderr, "ERROR: NO INFORMATION ABOUT GIVEN VIRUS\n\n");
        return -1;
    }

    int vaccinated = bloomExists(vir->vaccinated_bloom, citizenID);
    printf("%s\n\n", vaccinated? "MAYBE" : "NOT VACCINATED");
    return vaccinated;
}

// check if citizen exists in virus skiplist
void vaccineStatus(char *citizenID, Virus vir, HTHash viruses){
    if(vir == NULL){
        // no specific virus given, print all
        for(int i = 0; i < viruses->curSize; i++){
            for(Listptr l = viruses->ht[i]->next; l != l->tail; l = l->next){
                Virus vir = ((HTEntry)(l->value))->item;
                char *vname = vir->name;
                VaccRecord rec = skipGet(vir->vaccinated_persons, citizenID);
                if(rec){
                    printf("%s YES %02d-%02d-%04d\n\n", vname, rec->date->day, 
                           rec->date->month, rec->date->year);
                }else{
                    printf("%s NO\n\n", vname);
                }
            }
        }

        return;
    }

    // print specific virus info
    VaccRecord rec = skipGet(vir->vaccinated_persons, citizenID);

    if(rec){
        printf("VACCINATED ON %02d-%02d-%04d\n\n", rec->date->day, rec->date->month, rec->date->year);
    }else{
        printf("NOT VACCINATED\n\n");
    }
}

// prints every non vaccinated person of a specific virus 
void list_nonVaccinated_Persons(Virus v, HTHash countries){
    // no records about virus
    if(v == NULL){
        printf("NO RECORDS FOUND\n\n");
    }else{
        Skiplist skip = v->not_vaccinated_persons;
        
        for(skipNode snode = skip->dummy->forward[0]; 
            snode != NULL; snode = snode->forward[0]){

            Person per = ((VaccRecord)(snode->item))->per;
            printf("%s %s %s %s %d\n", per->citizenID, per->firstName,
                    per->lastName, per->country->name, per->age);
        }
        printf("\n");
    }
}

void populationStatus(Virus vir, Date d1, Date d2, HTHash countries, char *countryName){
    // error if virus not in records
    if(vir == NULL){
        fprintf(stderr, "ERROR: NO INFORMATION ABOUT VIRUS\n\n");
        return;
    }

    // error if country not in records
    if(!HTExists(countries, countryName)){
        fprintf(stderr, "ERROR: NO INFORMATION ABOUT COUNTRY\n\n");
    }

    // error if only one date was given
    if((d1 == NULL)^(d2 == NULL)){
        fprintf(stderr, "ERROR: INCORRECT SYNTAX, SEE /help\n\n");
        return;
    }

    int dates_given = 1;

    if(d1 == NULL){
        dates_given = 0;
    }

    Skiplist vacc = vir->vaccinated_persons;
    
    for(skipNode snode = vacc->dummy->forward[0];
        snode != NULL; snode = snode->forward[0]){

        Date date = ((VaccRecord)(snode->item))->date;
        Person per = ((VaccRecord)(snode->item))->per;
        Country country = per->country;

        if(countryName != NULL && strcmp(country->name, countryName)){
            continue;
        }

        if(dates_given){
            if(compareDates(date, d1) >= 0 && compareDates(date, d2) <= 0){
                country->popCounter += 1;
            }
        }else{
            country->popCounter += 1;
        }
    }

    for(int i = 0; i < countries->curSize; i++){
		for(Listptr l = countries->ht[i]->next; l != l->tail; l = l->next){
            Country country = ((HTEntry)(l->value))->item;

            if(countryName != NULL && strcmp(country->name, countryName)){
                continue;
            }

            char *name = country->name;
            int vaccPopulation = country->popCounter;
            int population = country->population;
            printf("%s %d %0.2f%%\n", name, vaccPopulation,\
                   (float)vaccPopulation*100/(float)population);

            // set counter back to zero
            country->popCounter = 0;

		}
	}
    printf("\n");
}

void popStatusByAge(Virus vir, Date d1, Date d2, HTHash countries, char *countryName){
    // error if virus not in records
    if(vir == NULL){
        fprintf(stderr, "ERROR: NO INFORMATION ABOUT VIRUS\n\n");
        return;
    }

    // error if country not in records
    if(countryName != NULL && !HTExists(countries, countryName)){
        fprintf(stderr, "ERROR: NO INFORMATION ABOUT COUNTRY\n\n");
    }

    // error if only one date was given
    if((d1 == NULL)^(d2 == NULL)){
        fprintf(stderr, "ERROR: INCORRECT SYNTAX, SEE /help\n\n");
        return;
    }

    int dates_given = 1;

    if(d1 == NULL){
        dates_given = 0;
    }

    Skiplist vacc = vir->vaccinated_persons;
    
    for(skipNode snode = vacc->dummy->forward[0];
        snode != NULL; snode = snode->forward[0]){

        Date date = ((VaccRecord)(snode->item))->date;
        Person per = ((VaccRecord)(snode->item))->per;
        Country country = per->country;

        if(countryName != NULL && strcmp(country->name, countryName)){
            continue;
        }

        if(dates_given){
            if(compareDates(date, d1) >= 0 && compareDates(date, d2) <= 0){
                incrementAgeCounter(country, per->age);
            }
        }else{
            incrementAgeCounter(country, per->age);
        }
    }

    for(int i = 0; i < countries->curSize; i++){
		for(Listptr l = countries->ht[i]->next; l != l->tail; l = l->next){
            Country country = ((HTEntry)(l->value))->item;

            if(countryName != NULL && strcmp(country->name, countryName)){
                continue;
            }

            char *name = country->name;
            int *agePopulation = country->agePopulation;
            int *ageCounter = country->ageCounter;
            float percentages[4];
            for(int i = 0; i < 4; i++){
                if(agePopulation[i] == 0){
                    percentages[i] = 0;
                    continue;
                }

                percentages[i] = (float)ageCounter[i]*100/(float)agePopulation[i];
            }

            printf("%s\n", name);
            printf("0-19 %d %0.2f%%\n", ageCounter[0], percentages[0]);
            printf("20-39 %d %0.2f%%\n", ageCounter[1], percentages[1]);
            printf("40-59 %d %0.2f%%\n", ageCounter[2], percentages[2]);
            printf("60+ %d %0.2f%%\n\n", ageCounter[3], percentages[3]);

            // set counter back to zero
            for(int i = 0; i < 4; i++){
                ageCounter[i] = 0;
            }

		}
	}
}

// prints a description of every available command
void printHelp(void){

    printf("\t\tCommands\n");
    printf("\t\t--------\n\n");

    printf("/vaccineStatusBloom citizenID virusName\n");
    printf("\tChecks bloom filter to see if citizenID is\
 vaccinated against the given virus\n\n");

    printf("/vaccineStatus citizenID virusName\n");
    printf("\tChecks if citizen is vaccinated against given virus\n\n");

    printf("/vaccineStatus citizenID\n");
    printf("\tChecks vaccine status of citizen for every known virus\n\n");

    printf("/populationStatus [country] virusName [date1 date2]\n");
    printf("\tPrints vaccine statistics of all countries for a given virus\n");
    printf("\tIf a country is given, statistics are printed for the specific country only\n");
    printf("\tIf a start date and end date are given,\n\tonly records within that timeline are\
 taken into account\n\n");

    printf("/popStatusByAge [country] virusName [date1 date2]\n");
    printf("\tPrints vaccine statistics by age group of all countries for a given virus\n");
    printf("\tIf a country is given, statistics are printed for the specific country only\n");
    printf("\tIf a start date and end date are given,\n\tonly records within that timeline are\
 taken into account\n\n");

    printf("/insertCitizenRecord citizenID firstName lastName country age virusName YES/NO [date]\n");
    printf("\tInserts new citizen record or updates the existing one\n\n");

    printf("/vaccinateNow citizenID firstName lastName country age virusName\n");
    printf("\tVaccinates given citizen if not already vaccinated\n\n");

    printf("/list-nonVaccinated-Persons virusName\n");
    printf("\tPrints all citizens who are not vaccinated against given virus\n\n");

    printf("/exit\n");
    printf("\tExits program\n\n");
}
