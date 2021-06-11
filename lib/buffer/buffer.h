// Checks if a buffer is empty
int bufferEmpty(char *buff, int buffsize);

// Gets last delimiter position in buffer
int getLastDel(char *buff, int buffsize);

// Checks if a string fits in a buffer
int strFits(char *buff, int buffsize, char *str);

// Inserts string to buffer
void buffInsert(char *buff, int buffsize, char *str);

// Remove last string from buffer and return it
char *buffGetLast(char *buff, int buffsize);
