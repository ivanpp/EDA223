#ifndef APPLICATION_API
#define APPLICATION_API

#include <stdbool.h>

#define MAX_BUFFER_SIZE 5
#define MAX_HISTORY_SIZE 5
#define NHISTORY 3

#define KEY_MAX 5
#define KEY_MIN -5
// pre-computed periods, [-15, 16]
#define PERIODS_IDX_DIFF -15
const int PERIODS [32] = {2702, 2551, 2407, 2272, 2145, 2024, 1911, 1803, /*-8*/
						  1702, 1607, 1516, 1431, 1351, 1275, 1203, 1136 /*0*/, 
						  1072, 1012, 955, 901, 851, 803, 758, 715 /*8*/, 
						  675, 637, 601, 568, 536, 506, 477, 450 /*16*/};

///@brief   define tone generator deadline 
#define TONE_GEN_DEADLINE USEC(100)

///@brief   define background Load deadline 
#define BGLOAD_DEADLINE USEC(1300)


typedef struct {
    Object super;
    int count; // counter for history[]
    int index; // index serves the buffer[]
    int sum; // current sum
    char c; //
    char buffer[MAX_BUFFER_SIZE];  // input buffer
    int history[MAX_HISTORY_SIZE]; // history
    int sortedh[MAX_HISTORY_SIZE]; // sorted-history

    /// @brief  member variable to check if deadline is enabled or disabled : part1_task3
    bool isDeadlineEnabled;

    ///@brief   tone-generator specific deadline 
    int toneGenDeadline;

    /// @brief  bgLoad specific deadline
    int bgLoadDeadline;
} App;

#define initApp() \
    { initObject(), 0, 0, 0, 'X', {},{},{},true,TONE_GEN_DEADLINE,BGLOAD_DEADLINE }

void reader(App*, int);
void receiver(App*, int);
void nhistory(App*, int);
void clearbuffer(App*, int);
void clearhistory(App*, int);

#endif