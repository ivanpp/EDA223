#include "TinyTimber.h"
#include "application.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

App app = initApp();

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

void nhistory(App *self, int val) {
    int far_idx, close_idx, median, count, farrr_idx, idx;
    idx   = self->count % MAX_HISTORY_SIZE; // current index
    self->history[idx] = val;
    count = self->count++;
    int n = NHISTORY;
    // n-median
    n = n > self->count ? self->count : n;
    far_idx   = idx - (int) n / 2;
    close_idx = idx - (int) (n - 1) / 2;
    far_idx   = (far_idx   % MAX_HISTORY_SIZE + MAX_HISTORY_SIZE) % MAX_HISTORY_SIZE;
    close_idx = (close_idx % MAX_HISTORY_SIZE + MAX_HISTORY_SIZE) % MAX_HISTORY_SIZE;
    median    = (int) ((self->history[close_idx]+self->history[far_idx]) / 2);
    // sum
    self->sum += val;
    farrr_idx = (count - NHISTORY) % MAX_HISTORY_SIZE;
    if (farrr_idx >= 0) self->sum -= self->history[farrr_idx];
    // display info
    count = self->count > MAX_HISTORY_SIZE ? MAX_HISTORY_SIZE : self->count;
    char temp [128] = {};
    snprintf(temp, 128, \
            "[%d/%d]: \'%d\' is stored, %d-history -> median: %d, sum: %d\n",
            count, MAX_HISTORY_SIZE, val, NHISTORY, median, self->sum);
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
}

int main() {
    INSTALL(&sci0, sci_interrupt, SCI_IRQ0);
	INSTALL(&can0, can_interrupt, CAN_IRQ0);
    TINYTIMBER(&app, startApp, 0);
    return 0;
}
