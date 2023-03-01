#include "TinyTimber.h"
#include "application.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include "toneGenerator.h"
#include "backgroundLoad.h"
#include "musicPlayer.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

App app = initApp();

BackgroundLoad backgroundLoad = initBackgroundLoad();
MusicPlayer musicPlayer = initMusicPlayer();

Serial sci0 = initSerial(SCI_PORT0, &app, reader);

Can can0 = initCan(CAN_PORT0, &app, receiver);

int brotherJohnIndices [32] = {0, 2, 4, 0,
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

// Get the value from buffer, then reset it
int parseValue(App *self, int unused) {
    self->buffer[self->index] = '\0';
    self->index = 0;
    return atoi(self->buffer);
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
    // create char inside switch case (only claim when needed)
    // think about the execution time
    // naming convention: funcInfo
    //char debugInfo[64] = { }; // for debuging
    int currentLoad;
    int volPercentage; // [0, 10]
    int val; // 
    switch (c)
    {
        /* display helper */
        case '\n':;
            char guide [1024] = {};
            snprintf(guide, 1024, 
                "-----------------------------------------------\n"
                "Try input some number:\n"
                "Maximum individual integer length: %d\n"
                "%d-history of median & sum will be shown\n"
                "-----------------------------------------------\n"
                "press \'-\' and \'0\'-\'9\' to input\n"
                "press \'e\' to end input and save\n"
                "press \'backspace\' to discard current input\n"
                "press \'f\' to erase the history\n"
                /* CHANGE TEXT ENCODING OF TERMINAL TO 'utf-8' */
                "press arrow-left to decrease bg load\n"
                //"press \'←\' to decrease bg load\n"
                "press arrow-right to increase bg load\n"
                //"press \'→\' to increase bg load\n"
                "press arrow-up to volumn-up\n"
                //"press \'↑\' to volumn-up\n"
                "press arrow-down to volumn-down\n"
                //"press \'↓\' to volumn-down\n"
                "press \'d\' to toggle deadline, default: deadline disabled\n"
                "------------------MUSIC PLAYER------------------\n"
                "press \'k\' to set the key\n"
                "press \'t\' to set the tempo\n"
                "press \'m\' to mute/unmute\n"
                "\n"
                "press \'enter\' to display this helper again\n"
                "\n",
                MAX_BUFFER_SIZE - 1,
                NHISTORY);
            SCI_WRITE(&sci0, guide);
            break;
        /* CRLF compatible */
        case '\r':
            break;
        /* valid integer input */
        case '-':
        case '0' ... '9':;
            SCI_WRITE(&sci0, "Input stored in buffer: \'");
            SCI_WRITECHAR(&sci0, c);
            SCI_WRITE(&sci0, "\'\n");
            self->buffer[self->index] = c;
            self->index = (self->index + 1) % MAX_BUFFER_SIZE;
            break;
        /* end of integer */
        case 'e':
        case 'E':;
            val = parseValue(self, /*unused*/0);
            nhistory(self, val);
            break;
        /* backspace */
        case '\b':
            clearbuffer(self, 0);
            SCI_WRITE(&sci0, "Input buffer cleared, nothing saved\n");
            break;
        /* set freq: [1,4000]*/
        case 'q':
        case 'Q':;
            char setFreqInfo [48] = { };
            val = parseValue(self, /*unused*/0);
            if (val < 1 || val > MAX_FREQ) {
                snprintf(setFreqInfo, 48,
                    "Freq must be: [%d, %d] (attempt: %d)\n", 1, MAX_FREQ, val);
                SCI_WRITE(&sci0, setFreqInfo);
            }
            int freq = SYNC(&toneGenerator, setFrequency, val);
            snprintf(setFreqInfo, 48, "Frquency set to: %d Hz\n", freq);
            SCI_WRITE(&sci0, setFreqInfo);
            break;
/*       case 'v':
            self->buffer[self->index] = '\0';
            int volume = atoi(self->buffer);
            self->index = 0;
            ASYNC(&toneGenerator, setVolume, volume);
            break; 
*/
        /* VOLUME CONTROL: mute/unmute, vol-down, vol-up*/
        /* toggle audio */
        case 'm':
        case 'M':;
            char volMuteInfo [16] = { };
            SYNC(&toneGenerator, toggleAudio, 0);
            if (toneGenerator.isMuted) snprintf(volMuteInfo, 64, "<X  muted\n");
            else snprintf(volMuteInfo, 64, "<)  unmuted\n");
            SCI_WRITE(&sci0, volMuteInfo);
            break;
        /* arrow-down: volumn down */
        case 0x1f:;
            char volDownInfo [32] = { };
            SYNC(&toneGenerator, adjustVolume, -1);
            snprintf(volDownInfo, 64, "<)  [-----------]%d/%d\n", 
                toneGenerator.volume, SAFE_VOLUME);
            volPercentage = (10 * toneGenerator.volume) / SAFE_VOLUME;
            volDownInfo[volPercentage+5] = '|';
            SCI_WRITE(&sci0, volDownInfo);
            break;
        /* arrow-up, volumn up */
        case 0x1e:;
            char volUpInfo [32] = { };
            SYNC(&toneGenerator, adjustVolume, 1);
            snprintf(volUpInfo, 64, "<)) [-----------]%d/%d\n", 
                toneGenerator.volume, SAFE_VOLUME);
            volPercentage = (10 * toneGenerator.volume) / SAFE_VOLUME;
            volUpInfo[volPercentage+5] = '|';
            SCI_WRITE(&sci0, volUpInfo);
            break;
        /* BACKGROUND LOAD CONTROL: load-down, load-up */
        /* arrow-left, bgLoad down */
        case 0x1c:;
            char loadDwonInfo [32] = { };
            currentLoad = SYNC(&backgroundLoad, adjustLoad, -LOAD_STEP);
            snprintf(loadDwonInfo, 32, "Current background load: %d\n", currentLoad);
            SCI_WRITE(&sci0, loadDwonInfo);
            break;
        /* arrow-right, bgLoad up */
        case 0x1d:;
            char loadUpInfo [32] = { };
            currentLoad = SYNC(&backgroundLoad, adjustLoad, +LOAD_STEP);
            snprintf(loadUpInfo, 32, "Current background load: %d\n", currentLoad);
            SCI_WRITE(&sci0, loadUpInfo);
            break;
        /* DEADLINE CONTROL: enable/disable */
        /* Toggle deadline flags for each task */
        case 'd':
        case 'D':;
            char deadlineInfo [32] = {};
            int tgStatus, bgStatus;
            tgStatus = SYNC(&toneGenerator, toggleDeadlineTG, /*unused*/ 0);
            bgStatus = SYNC(&backgroundLoad, toggleDeadlineBG, /*unused*/ 0);
            if (tgStatus && bgStatus) {
                snprintf(deadlineInfo, 32, "Deadline enabled\n");
            } else{
                snprintf(deadlineInfo, 32, "Deadline disabled\n");
            }
            SCI_WRITE(&sci0, deadlineInfo);
            break;
        /* MUSIC PLAYER CONTROL*/
        /* change key */
        case 'k':
        case 'K':;
            char musicKeyInfo [48] = { };
            int key = parseValue(self, /*unused*/0);
            if (key < KEY_MIN || key > KEY_MAX){
                snprintf(musicKeyInfo, 48, 
                        "Key must be: [%d, %d] (attempt: %d)\n", KEY_MIN, KEY_MAX, key);
                SCI_WRITE(&sci0, musicKeyInfo);
            }
            key = SYNC(&musicPlayer, setKey, key);
            snprintf(musicKeyInfo, 32, "Key set to: %d\n", key);
            SCI_WRITE(&sci0, musicKeyInfo);
            break;
        /* change tempo */
        case 't':
        case 'T':;
            char musicTempoInfo [48] = { };
            int tempo = parseValue(self, /*unused*/0);
            if (tempo < TEMPO_MIN || tempo > TEMPO_MAX){
                snprintf(musicTempoInfo, 48, 
                        "Tempo must be: [%d, %d] (attempt: %d)\n", TEMPO_MIN, TEMPO_MAX, tempo);
                SCI_WRITE(&sci0, musicTempoInfo);
            }
            tempo = SYNC(&musicPlayer, setTempo, tempo);
            snprintf(musicTempoInfo, 32, "Tempo set to %d\n", tempo);
            SCI_WRITE(&sci0, musicTempoInfo);
            break;
        /* erase nhistory */
        case 'f':
        case 'F':;
            clearhistory(self, 0);
            clearbuffer(self, 0);
            char tempf[32] = {};
            snprintf(tempf, 32, "%d-history has been erased\n", NHISTORY);
            SCI_WRITE(&sci0, tempf);
            break;
        /* period lookup */
        case 'p':
        case 'P':
            // TODO: use strcat
            val = parseValue(self, /*unused*/0);
            char lookupInfo[32] = {};
            snprintf(lookupInfo, 32, "Key: %d\n", val);
            SCI_WRITE(&sci0, lookupInfo);
            if (val < KEY_MIN || val > KEY_MAX){
                snprintf(lookupInfo, 32, "Invalid! Key must be: [%d, %d]\n", KEY_MIN, KEY_MAX);
                SCI_WRITE(&sci0, lookupInfo);
            } else{
                int index, period;
                for (int i = 0; i < 32; i++){
                    index = brotherJohnIndices[i] + val - PERIODS_IDX_DIFF;
                    period = PERIODS[index];
                    snprintf(lookupInfo, 32, "%d ", period);
                    SCI_WRITE(&sci0, lookupInfo);
                }
                SCI_WRITE(&sci0, "\n");
            }
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

    /* introduce deadline to tone generator */
    BEFORE(toneGenerator.toneGenDeadline,&toneGenerator, playTone, 10);

    /* introduce deadline to backgroundLoad task */
    BEFORE(backgroundLoad.bgLoadDeadline,&backgroundLoad, loadLoop, 10);

    //SYNC(&musicPlayer, playMusic, 0);
    ASYNC(&musicPlayer, playMusic, 0);
}

int main() {
    INSTALL(&sci0, sci_interrupt, SCI_IRQ0);
	INSTALL(&can0, can_interrupt, CAN_IRQ0);
    TINYTIMBER(&app, startApp, 0);
    return 0;
}
