#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include <stdlib.h>
#include <stdio.h>


#define MAX_BUFFER_SIZE 4

typedef struct {
    Object super;
    int count; // haven't been used yet
    int index;
    char c; // haven't been used yet
    char buffer[MAX_BUFFER_SIZE]; // MAX_BUFFER_SIZE * 1B
} App;

App app = { initObject(), 0, 0, 'X', {} };

void reader(App*, int);
void receiver(App*, int);

Serial sci0 = initSerial(SCI_PORT0, &app, reader);

Can can0 = initCan(CAN_PORT0, &app, receiver);

void receiver(App *self, int unused) {
    CANMsg msg;
    CAN_RECEIVE(&can0, &msg);
    SCI_WRITE(&sci0, "Can msg received: ");
    SCI_WRITE(&sci0, msg.buff);
}

void reader(App *self, int c) {
    switch (c)
    {
        case 'e':
        case 'E':
            self->buffer[self->index] = '\0';
            int val = atoi(self->buffer);
            char temp [MAX_BUFFER_SIZE] = {};
            int _len = snprintf(temp, 32, "%d\n", val);
            self->index = 0;
            SCI_WRITE(&sci0, temp);
            break;
        case '-':
        case '0' ... '9':
            SCI_WRITE(&sci0, "Input stored in buffer: \'");
            SCI_WRITECHAR(&sci0, c);
            SCI_WRITE(&sci0, "\'\n");
            self->buffer[self->index] = c;
            self-> index = (self->index + 1) % MAX_BUFFER_SIZE;
            break;
        case '\n':
        ;
            char guide [256] = {};
            int len = snprintf(guide, 256,  "-----------------------------------------------\n"
                                            "Try input some number:\n"
                                            "Maximum individual integer length: %d\n"
                                            "-----------------------------------------------\n"
                                            "press \'e\' to end the number:\n"
                                            "press \'enter\' to display this helper again\n",
                                            MAX_BUFFER_SIZE - 1);
            SCI_WRITE(&sci0, guide);
            break;
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
