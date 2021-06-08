#include "../lib/bloomfilter/bloomfilter.h" 
#include "../lib/skiplist/skiplist.h"
#include "../lib/hashtable/htInterface.h"
#include "../lib/date/date.h"


// Database
struct databasestr{
    HTHash viruses;
    HTHash persons;
    HTHash countries;
    char *cyclicBuff;
    int cyclicBufferSize;
    int sizeOfBloom;
};

typedef struct databasestr *Database;

// Virus
struct virusstr{
    char *name;
    BloomFilter vaccinated_bloom;
    Skiplist vaccinated_persons;
    Skiplist not_vaccinated_persons;
};

typedef struct virusstr *Virus;

Virus newVirus(char *, int, int, float);
void destroyVirus(Virus);

// Country
struct countrystr{
    char *name;
    int popCounter; // counter for national population
    int population;
    int ageCounter[4]; // counters for age-based population
    int agePopulation[4];
};

typedef struct countrystr *Country;

Country newCountry(char *);
void incrementAgePopulation(Country, int);

// Person
struct personstr{
    char *citizenID;
    char *firstName;
    char *lastName;
    Country country;
    int age;
};

typedef struct personstr *Person;

// Vaccine Record
struct vaccrecstr{
    Person per;
    Date date;
};

typedef struct vaccrecstr *VaccRecord;

void deleteRecord(VaccRecord);

// Interface
void insertCitizenRecord(VaccRecord, Virus);
int vaccineStatusBloom(char *, Virus);
void vaccineStatus(char *, Virus, HTHash);
void list_nonVaccinated_Persons(Virus, HTHash);
void populationStatus(Virus, Date, Date, HTHash, char *);
void popStatusByAge(Virus, Date, Date, HTHash, char *);
void printHelp(void);
