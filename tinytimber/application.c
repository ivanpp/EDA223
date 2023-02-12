#include "TinyTimber.h"
#include "application.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include "toneGenerator.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

App app = initApp();

ToneGenerator toneGenerator = initToneGenerator();

Serial sci0 = initSerial(SCI_PORT0, &app, reader);

Can can0 = initCan(CAN_PORT0, &app, receiver);

int brotherJohn [32] = {0, 2, 4, 0,
                        0, 2, 4, 0,
                        4, 5, 7,
                        4, 5, 7,
                        7, 9, 7, 5, 4, 0,
                        7, 9, 7, 5, 4, 0,
                        0, -5, 0,
                        0, -5, 0};

void receiver(App *self, int unused) {
    CANMsg msg;
    CAN_RECEIVE(&can0, &msg);
    SCI_WRITE(&sci0, "Can msg received: ");
    SCI_WRITE(&sci0, msg.buff);
}

void clearbuffer(App *self, int unused) {
    for (int i = 0; i < MAX_BUFFER_SIZE - 1; i++){
        self->buffer[i] = '\0';
    };
    self->index = 0;
}

void clearhistory(App *self, int unused) {
    for (int i = 0; i < MAX_HISTORY_SIZE - 1; i++){
        self->history[i] = 0;
    }
    self->count = 0;
    self->sum   = 0;
}

// https://devdocs.io/c/algorithm/qsort
int compare_ints(const void* a, const void* b){
    int arg1 = *(const int*)a;
    int arg2 = *(const int*)b;
    return (arg1 > arg2) - (arg1 < arg2);
}

void nhistory(App *self, int val) {
    int n = NHISTORY;
    int mleft_idx, mright_idx, median, count, tail, head;
    head   = self->count % MAX_HISTORY_SIZE; // current index (head)
    self->history[head] = val;
    count = ++self->count; // current count
    // min(n, count)-history will be applied
    n = n > count ? count : n; 
    tail = (count - NHISTORY) % MAX_HISTORY_SIZE; // (tail), start from negative, but never negative again
    // memcpy and sort
    if (tail > head){
        self->sum   = self->sum - self->history[tail-1]; // first in, first deducted from sum
        int len_t2e = MAX_HISTORY_SIZE-tail;
        int len_s2h = n - len_t2e;
        memcpy(self->sortedh        , self->history+tail, len_t2e*sizeof(int));
        memcpy(self->sortedh+len_t2e, self->history     , len_s2h*sizeof(int));
    } else if (tail == 0){
        if (count > n) self->sum = self->sum - self->history[MAX_HISTORY_SIZE-1];
        memcpy(self->sortedh, self->history+tail, n*sizeof(int));
    } else if (tail > 0){
        self->sum = self->sum - self->history[tail-1];
        memcpy(self->sortedh, self->history+tail, n*sizeof(int));
    } else{
        memcpy(self->sortedh, self->history, n*sizeof(int));
    }
    qsort(self->sortedh, n, sizeof(int), compare_ints);
    mleft_idx   = (n - 1) - (int) n / 2;
    mright_idx  = (n - 1) - (int) (n - 1) / 2;
    median      = (int) ((self->sortedh[mright_idx] + self->sortedh[mleft_idx]) / 2);
    self->sum += val;
    count = count > MAX_HISTORY_SIZE ? MAX_HISTORY_SIZE : count; // for print
    char temp [128] = {};
    snprintf(temp, 128, \
            "[%d/%d]: \'%d\' is stored, %d-history -> sum: %d, median: %d\n",
            count, MAX_HISTORY_SIZE, val, n, self->sum, median);
    SCI_WRITE(&sci0, temp);
}

void reader(App *self, int c) {
    switch (c)
    {
        case 'e': // end of the integer
        case 'E':;
            self->buffer[self->index] = '\0';
            int val = atoi(self->buffer);
            self->index = 0;
            nhistory(self, val);
            break;
        case 'q':
            self->buffer[self->index] = '\0';
            int frequency = atoi(self->buffer);
            self->index = 0;
            ASYNC(&toneGenerator, setFrequency, frequency);
            break;
        case 'v':
            self->buffer[self->index] = '\0';
            int volume = atoi(self->buffer);
            self->index = 0;
            ASYNC(&toneGenerator, setVolume, volume);
            break;
        case 'n':
            ASYNC(&toneGenerator, adjustVolume, -1);
            break;
        case 'm':
            ASYNC(&toneGenerator, adjustVolume, 1);
            break;
        case 'f': // erase nhistory
        case 'F':;
            clearhistory(self, 0);
            clearbuffer(self, 0);
            char tempf[32] = {};
            snprintf(tempf, 32, "%d-history has been erased\n", NHISTORY);
            SCI_WRITE(&sci0, tempf);
            break;
        case 'p': // period lookup
        case 'P':
            // TODO: use strcat
            self->buffer[self->index] = '\0';
            int key = atoi(self->buffer);
            self->index = 0;
            char tempp[32] = {};
            snprintf(tempp, 32, "Key: %d\n", key);
            SCI_WRITE(&sci0, tempp);
            if (key < KEY_MIN || key > KEY_MAX){
                snprintf(tempp, 32, "Invalid! Key must be: [%d, %d]\n", KEY_MIN, KEY_MAX);
                SCI_WRITE(&sci0, tempp);
            } else{
                int index, period;
                for (int i = 0; i < 32; i++){
                    index = brotherJohn[i] + key - PERIODS_IDX_DIFF;
                    period = PERIODS[index];
                    snprintf(tempp, 32, "%d ", period);
                    SCI_WRITE(&sci0, tempp);
                }
                SCI_WRITE(&sci0, "\n");
            }
            break;
        case '-':
        case '0' ... '9':;
            SCI_WRITE(&sci0, "Input stored in buffer: \'");
            SCI_WRITECHAR(&sci0, c);
            SCI_WRITE(&sci0, "\'\n");
            self->buffer[self->index] = c;
            self->index = (self->index + 1) % MAX_BUFFER_SIZE;
            break;
        case '\n':;
            char guide [512] = {};
            snprintf(guide, 512,  "-----------------------------------------------\n"
                                  "Try input some number:\n"
                                  "Maximum individual integer length: %d\n"
                                  "%d-history of median & sum will be shown\n"
                                  "-----------------------------------------------\n"
                                  "press \'-\' and \'0\'-\'9\' to input\n"
                                  "press \'e\' to end input and save\n"
                                  "press \'backspace\' to discard current input\n"
                                  "press \'f\' to erase the history\n"
                                  "press \'enter\' to display this helper again\n"
                                  "\n",
                                  MAX_BUFFER_SIZE - 1,
                                  NHISTORY);
            SCI_WRITE(&sci0, guide);
            break;
        case '\b':
            clearbuffer(self, 0);
            SCI_WRITE(&sci0, "Input buffer cleared, nothing saved\n");
        case '\r':
            break;
        default:
            SCI_WRITE(&sci0, "Input received: \'");
            SCI_WRITECHAR(&sci0, c);
            SCI_WRITE(&sci0, "\'\n");
            break;
    }
}

void startApp(App *self, int arg) {
    CANMsg msg;

    CAN_INIT(&can0);
    SCI_INIT(&sci0);
    SCI_WRITE(&sci0, "Hello from RTS C1...\n");

    msg.msgId = 1;
    msg.nodeId = 1;
    msg.length = 6;
    msg.buff[0] = 'H';
    msg.buff[1] = 'e';
    msg.buff[2] = 'l';
    msg.buff[3] = 'l';
    msg.buff[4] = 'o';
    msg.buff[5] = 0;
    CAN_SEND(&can0, &msg);

    ASYNC(&toneGenerator, playTone, 10);
}

int main() {
    INSTALL(&sci0, sci_interrupt, SCI_IRQ0);
	INSTALL(&can0, can_interrupt, CAN_IRQ0);
    TINYTIMBER(&app, startApp, 0);
    return 0;
}
