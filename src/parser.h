#pragma once

#include "../lib/hashtable/htInterface.h"

void parseExecutableParameters(int, char **, char **, int *);
void parseInputFile(char *, int, HTHash, HTHash, HTHash);
void inputLoop(HTHash, HTHash, HTHash, int);
