#include <stdio.h>
#include <stdlib.h>
#include "date.h"

#define ABS(a) a >= 0? a : -a

// d1 > d2  ->  1
// d1 == d2 ->  0
// d1 < d2  -> -1
int compareDates(Date d1, Date d2){
    if(d1->year > d2->year){
        return 1; 
    }else if(d1->year < d2->year){
        return -1;
    }

    if(d1->month > d2->month){
        return 1;
    }else if(d1->month < d2->month){
        return -1;
    }

    if(d1->day > d2->day){
        return 1;
    }else if(d1->day < d2->day){
        return -1;
    }

    return 0;
}

// Difference of two dates in days
int getDiffDate(Date d1, Date d2){
    int days1 = d1->day + d1->month*30 + d1->year*360;
    int days2 = d2->day + d2->month*30 + d2->year*360;
    return ABS(days1-days2);
}
