#ifndef APPLICATION_API
#define APPLICATION_API

#define MAX_BUFFER_SIZE 4
#define MAX_HISTORY_SIZE 8
#define NHISTORY 3

#define KEY_MAX 5
#define KEY_MIN -5
// pre-computed periods, [-15, 16]
#define PERIODS_IDX_DIFF -15
const int PERIODS [32] = {2717, 2564, 2415, 2272, 2145, 2032, 1915, 1805 /*-8*/, 
                          1706, 1607, 1519, 1432, 1355, 1278, 1204, 1136 /*0*/, 
                          1072, 1014, 956, 902, 851, 803, 758, 716 /*8*/, 
                          676, 638, 602, 568, 536, 506, 478, 451 /*16*/};


typedef struct {
    Object super;
    int count; // counter for history[]
    int index; // index serves the buffer[]
    int sum;
    char c; // haven't been used yet
    char buffer[MAX_BUFFER_SIZE];
    int history[MAX_HISTORY_SIZE];
} App;

#define initApp() \
    { initObject(), 0, 0, 0, 'X', {} }

void reader(App*, int);
void receiver(App*, int);
void nhistory(App*, int);
void clearbuffer(App*, int);
void clearhistory(App*, int);

#endif