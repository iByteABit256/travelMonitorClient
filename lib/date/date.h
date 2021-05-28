struct datestr{
    unsigned int day;
    unsigned int month;
    unsigned int year;
};

typedef struct datestr *Date;

int compareDates(Date, Date);
int getDiffDate(Date, Date);
