#ifndef APPLICATION_API
#define APPLICATION_API

#include <stdbool.h>

#define MAX_BUFFER_SIZE 5
#define MAX_HISTORY_SIZE 5
#define NHISTORY 3

#define CONDUCTOR 1
#define MUSICIAN 0

// pre-computed periods, [-15, 16]
#define PERIODS_IDX_DIFF -15
const int PERIODS [32] = {2702, 2551, 2407, 2272, 2145, 2024, 1911, 1803, /*-8*/
						  1702, 1607, 1516, 1431, 1351, 1275, 1203, 1136 /*0*/, 
						  1072, 1012, 955, 901, 851, 803, 758, 715 /*8*/, 
						  675, 637, 601, 568, 536, 506, 477, 450 /*16*/};

int brotherJohnIndices [32] = {0, 2, 4, 0,
                            0, 2, 4, 0,
                            4, 5, 7,
                            4, 5, 7,
                            7, 9, 7, 5, 4, 0,
                            7, 9, 7, 5, 4, 0,
                            0, -5, 0,
                            0, -5, 0};

typedef struct {
    Object super;
    int count; // counter for history[]
    int index; // index serves the buffer[]
    int sum; // current sum
    int mode; // 1: conductor, 0: musician
    char buffer[MAX_BUFFER_SIZE];  // input buffer
    int history[MAX_HISTORY_SIZE]; // history
    int sortedh[MAX_HISTORY_SIZE]; // sorted-history
} App;

#define initApp() \
    { initObject(), 0, 0, 0, MUSICIAN, {},{},{} }

void reader(App*, int);
void receiver(App*, int);
void nhistory(App*, int);
void clearbuffer(App*, int);
void clearhistory(App*, int);
int parseValue(App*, int);

void sioDebug(App*, int);

#endif