#include "musicPlayer.h"
#include "toneGenerator.h"
#include "systemPorts.h"
#include "brotherJohn.h"

MusicPlayer musicPlayer = initMusicPlayer();


/* Single-player */

// set Key
int set_key(MusicPlayer *self, int key){
    key = key < KEY_MIN ? KEY_MIN : key;
    key = key > KEY_MAX ? KEY_MAX : key;
    self->key = key;
    return key;
}


// set tempo (bpm) for 'a'
int set_tempo(MusicPlayer *self, int tempo){
    tempo = tempo < TEMPO_MIN ? TEMPO_MIN : tempo;
    tempo = tempo > TEMPO_MAX ? TEMPO_MAX : tempo;
    int beatMult = 30 * 1000 / tempo;
    self->tempo   = tempo;
    self->beatMult = beatMult;
    return tempo;
}


// pause/unpause
int music_pause_unpause(MusicPlayer *self, int unused){
    // if music is stop
    if (self->hardStopped) {
        self->isStop = 0;
        self->hardStopped = 0;  
        SYNC(&toneGenerator, start_toneGen, 0);
        SYNC(&toneGenerator, play_tone, 0);
        play_music(self, 0);
    } else {
        self->isStop = 1;
        SYNC(&toneGenerator, stop_toneGen, 0);
    }
    return self->isStop;
}


int music_unpause(MusicPlayer *self, int unused){
    if (self->hardStopped) {
        self->isStop = 0;
        self->hardStopped = 0;  
        SYNC(&toneGenerator, start_toneGen, 0);
        SYNC(&toneGenerator, play_tone, 0);
        play_music(self, 0);
    }
    return self->isStop;
}


int music_pause(MusicPlayer *self, int unused){
    if (!self->hardStopped) {
        self->isStop = 1;
        SYNC(&toneGenerator, stop_toneGen, 0);
    }
    return self->isStop;
}


// stop/restart
int music_stop_start(MusicPlayer *self, int unused){
    // if music is stop
    if (self->hardStopped) {
         // reset the index, start from the begining
        self->isStop = 0;
        self->hardStopped = 0;
        SYNC(&toneGenerator, start_toneGen, 0);
        self->index = 0;
        SYNC(&toneGenerator, play_tone, 0);
        play_music(self, 0);
    } else{
        self->isStop = 1;
        SYNC(&toneGenerator, stop_toneGen, 0);
        self->index = 0;
    }
    return self->isStop;
}


int music_start(MusicPlayer *self, int unused){
    if (self->hardStopped) {
        self->isStop = 0;
        self->hardStopped = 0;
        SYNC(&toneGenerator, start_toneGen, 0);
        self->index = 0;
        SYNC(&toneGenerator, play_tone, 0);
        play_music(self, 0);
    }
    return self->isStop;
}


int music_stop(MusicPlayer *self, int unused){
    if (!self->hardStopped){
        self->isStop = 1;
        SYNC(&toneGenerator, stop_toneGen, 0);
        self->index = 0;
    }
    return self->isStop;
}


// scheduler
void play_music(MusicPlayer *self, int unused){
    // get next tone info
    if (self->isStop) {
        self->hardStopped = 1;
        return;
    }
    int period, tempo, beatLen;
    period = pianoPeriods[brotherJohn[self->index] + self->key - PERIODS_IDX_DIFF]; //Get period
    tempo = tempos[self->index]; // val can be 2, 4, 1
    beatLen = self->beatMult * tempo; // Get beatLen
    // LED: lit at the begining, unlit in the middle of a beat
    // since we stores int 2, 4, 1 for 1-beat, 2-beat, half-beat
    // we can use it to schedule LED too
    // first, start with the STUPID way
    switch(tempo){
        case(1):
            AFTER(MSEC(0), &sio0, sio_toggle, 0);
            break;
        case(2):
            AFTER(MSEC(0), &sio0, sio_write, 0); // lit
            AFTER(MSEC(beatLen/2), &sio0, sio_toggle, 0);
            break;
        case(4):
            AFTER(MSEC(0), &sio0, sio_write, 0); // lit
            AFTER(MSEC(1*beatLen/4), &sio0, sio_toggle, 0);
            AFTER(MSEC(2*beatLen/4), &sio0, sio_toggle, 0);
            AFTER(MSEC(3*beatLen/4), &sio0, sio_toggle, 0);
            break;
    }
    // set tone generator
    ASYNC(&toneGenerator, blank_tone, 0);
    ASYNC(&toneGenerator, set_period, period);
    AFTER(MSEC(50), &toneGenerator, unblank_tone, 0);
    // next tone
    self->index++;
    self->index = self->index % MUSIC_LENGTH;
    // call it self?
    AFTER(MSEC(beatLen), self, play_music, 0);
}


/* Multi-player */

/*

0       t1      play tone
t1      t1+t2   (next node) play next tone
t1+t2+d         start backup

*/
void play_index_tone(MusicPlayer *self, int idx){
    // 0. check index first
    if (idx < 0 || idx > MUSIC_LENGTH - 1) {
        char idxInfo[64];
        snprintf(idxInfo, 64, "WARN: invalid idx for song: %d\n", idx);
        SCI_WRITE(&sci0, idxInfo);
        return;
    }
    // 2. play the tone
    int period, tempo, beatLen;
    period = pianoPeriods[brotherJohn[idx] - PERIODS_IDX_DIFF];
    tempo = tempos[idx];
    beatLen = self->beatMult * tempo;
#ifdef DEBUG_MUSIC
    char debugInfo[64] = {};
    snprintf(debugInfo, 64, "play_i[%d]: period: %d, beatLen: %d\n", idx, period, beatLen);
    SCI_WRITE(&sci0, debugInfo);
#endif
    ASYNC(&toneGenerator, set_period, period);
    AFTER(MSEC(50), &toneGenerator, unblank_tone, 0);
    // 3. LED
    if (app.mode == CONDUCTOR) 
        sync_LED(self, idx);
    else {
        CANMsg msg;
        construct_can_message(&msg, MUSIC_SYNC_LED, network.conductorRank, idx);
        CAN_SEND(&can0, &msg); // >> sync_LED(idx)
    }
    // 3. schedule next tone, cancel the prev backup
    AFTER(MSEC(beatLen), self, play_index_tone_next, idx);
    // 4. schedule backup
    int nextTone, nextNode, backupTime;
    nextTone = (idx + 1) % MUSIC_LENGTH;
    nextNode = SYNC(&network, get_next_valid_node, 0);
    backupTime = (tempos[idx] + tempos[nextTone]) * self->beatMult + BACKUP_DELTA;
    if (nextNode != network.rank){
        self->backupMsg = AFTER(MSEC(backupTime), self, play_index_tone_next_backup, idx);
#ifdef DEBUG_MUSIC
        snprintf(debugInfo, 64, "Backup created, time left: %d\n", backupTime);
        SCI_WRITE(&sci0, debugInfo);
#endif
    }
}


// send idx (which note to play) to next node
void play_index_tone_next(MusicPlayer *self, int idx){
#ifdef DEBUG_MUSIC
    char debugInfo[64] = {};
    snprintf(debugInfo, 64, "play_n[%d]\n", idx);
    SCI_WRITE(&sci0, debugInfo);
#endif
    if(self->ensemble_stop)
        return;
    int nextTone, prevNode, nextNode;
    nextTone = (idx + 1) % MUSIC_LENGTH;
    nextNode = SYNC(&network, get_next_valid_node, 0);
    prevNode = SYNC(&network, get_prev_valid_node, 0);
    // 1. ask prev to cancel backup
    cancel_prev_backup(self, prevNode);
    // 2. ask next node to play
    if(nextNode == network.rank){
        play_index_tone(self, nextTone);
    }else{
        CANMsg msg;
        construct_can_message(&msg, MUSIC_PLAY_NOTE_IDX, nextNode, nextTone);
        CAN_SEND(&can0, &msg); // >> play_index_tone(idx++)
    }
    // 3. shut self
    SYNC(&toneGenerator, blank_tone, 0);
}


void cancel_prev_backup(MusicPlayer *self, int prev){
    // 1. cancel the prev's backup
    int prev_node = SYNC(&network, get_prev_valid_node, 0);
    if(prev_node != network.rank){
        CANMsg msg;
        construct_can_message(&msg, NODE_REMAIN_ACTIVE, prev_node, 0);
        CAN_SEND_WR(&can0, &msg); // >> cancel_backup()
#ifdef DEBUG
        char debugInfo[64];
        snprintf(debugInfo, 64, "Cancel the backup for Node: %d\n", prev);
        SCI_WRITE(&sci0, debugInfo);
#endif
    }
}


void play_index_tone_next_backup(MusicPlayer *self, int idx){
    ABORT(self->backupMsg);
    // 1. check if ensemble is stopped
    if(self->ensemble_stop)
        return;
    char backupInfo[64];
    snprintf(backupInfo, 64, "[PLAYER]: backup triggled [%d]\n", idx);
    SCI_WRITE(&sci0, backupInfo);
    // 2. start the detection program
    CANMsg msg;
    construct_can_message(&msg, DEBUG_OP, BROADCAST, 0); // test message
    if(CAN_SEND_WRN(&can0, &msg, 1)){
        SYNC(&network, node_logout, 0);
    } else {
        // detect each node
        SYNC(&network, detect_all_nodes, 0);
    }
    // 3. play next note as backup
    idx = (idx) % MUSIC_LENGTH;
#ifdef DEBUG_MUSIC
    char debugInfo[64];
    snprintf(debugInfo, 64, "play_b[%d]\n", idx);
    SCI_WRITE(&sci0, debugInfo);
#endif
    // resume music
    AFTER(MSEC(10), self, play_index_tone, idx);
}


void cancel_backup(MusicPlayer *self, int unused){
    ABORT(self->backupMsg);
}


void sync_LED(MusicPlayer *self, int idx){
    idx = idx < 0 ? 0 : idx;
    idx = idx > (MUSIC_LENGTH - 1) ? (MUSIC_LENGTH - 1) : idx;
    int tempo = tempos[idx];
    if (app.mode == CONDUCTOR){
        int beatLen = self->beatMult * tempo;
        switch(tempo){
        case(1):
            AFTER(MSEC(0), &sio0, sio_toggle, 0);
            break;
        case(2):
            AFTER(MSEC(0), &sio0, sio_write, 0); // lit
            AFTER(MSEC(beatLen/2), &sio0, sio_toggle, 0);
            break;
        case(4):
            AFTER(MSEC(0), &sio0, sio_write, 0); // lit
            AFTER(MSEC(1*beatLen/4), &sio0, sio_toggle, 0);
            AFTER(MSEC(2*beatLen/4), &sio0, sio_toggle, 0);
            AFTER(MSEC(3*beatLen/4), &sio0, sio_toggle, 0);
            break;
        default:
            SCI_WRITE(&sci0, "WARN: undefined beat length for LED controller\n");
            break;
        }
    }
}


void ensemble_ready(MusicPlayer *self, int unused){
    // setup ToneGenerator, ready to play, then blank it
    SYNC(&toneGenerator, start_toneGen, 0);
    SYNC(&toneGenerator, play_tone, 0);
    SYNC(&toneGenerator, blank_tone, 0);
    // ready to send can msg
    self->ensemble_stop = 0;
}


void ensemble_stop(MusicPlayer *self, int unused){
    SYNC(&toneGenerator, stop_toneGen, 0);
    self->ensemble_stop = 1;
}


// CONDUCTOR: start playing music the round-robin way
void ensemble_start_all(MusicPlayer *self, int unused){
    if(app.mode != CONDUCTOR){
        SCI_WRITE(&sci0, "[PLAYER ERR]: Ensemble can be only started by CONDUCTOR");
        return;
    }
    // get ready
    ensemble_ready(self, 0);
    CANMsg msg;
    construct_can_message(&msg, MUSIC_START_ALL, BROADCAST, 0);
    CAN_SEND(&can0, &msg); // >> ensemble_ready()
    // sync tempo, key
    set_tempo_all(self, self->tempo);
    set_key_all(self, self->key);
    // start from first valid node
    int firstNode = SYNC(&network, get_first_valid_node, unused);
    if (firstNode == network.rank)
        play_index_tone(self, 0);
    else{
        CANMsg msg;
        construct_can_message(&msg, MUSIC_PLAY_NOTE_IDX, firstNode, 0);
        CAN_SEND(&can0, &msg); // >> play_index_tone(0)
    }
}


// CONDUCTOR: stop ensemble
void ensemble_stop_all(MusicPlayer *self, int unused){
    CANMsg msg;
    construct_can_message(&msg, MUSIC_STOP_ALL, BROADCAST, 0);
    CAN_SEND(&can0, &msg); // >> ensemble_stop()
    ABORT(self->backupMsg);
    ensemble_stop(self, 0);
}


// CONDUCTOR: restart ensemble
void ensemble_restart_all(MusicPlayer *self, int unused){
    ensemble_stop_all(self, 0);
    // FIXME: use elegant way
    AFTER(SEC(1), &musicPlayer, ensemble_start_all, 0);
}


/* TODO the masked way*/
void play_music_masked(MusicPlayer *self, int unused){
    ;
}


int set_key_all(MusicPlayer *self, int key){
    key = set_key(self, key);
    CANMsg msg;
    construct_can_message(&msg, MUSIC_SET_KEY_ALL, BROADCAST, key);
    CAN_SEND(&can0, &msg); // >> set_key(key)
    return key;
}


int set_tempo_all(MusicPlayer *self, int tempo){
    tempo = set_tempo(self, tempo);
    CANMsg msg;
    construct_can_message(&msg, MUSIC_SET_TEMPO_ALL, BROADCAST, tempo);
    CAN_SEND(&can0, &msg); // >> set_tempo(tempo)
    return tempo;
}


// reset key, tempo for all boards
void reset_all(MusicPlayer *self, int unused){
    if(app.mode == CONDUCTOR){
        set_tempo_all(self, TEMPO_DEFAULT);
        set_key_all(self, KEY_DEFAULT);
        SCI_WRITE(&sci0, "[PLAYER]: reset tempo and key\n");
    }else
        SCI_WRITE(&sci0, "[PLAYER ERR]: reset_all only allowed by CONDUCTOR\n");
}


int toggle_music(MusicPlayer *self, int unused){
    if(app.mode == MUSICIAN){
        SCI_WRITE(&sci0, "[PLAYER]: toggled\n");
        SYNC(&toneGenerator, toggle_audio, 0);
        int muteStatus = toneGenerator.isMuted;
        if (muteStatus) {
            SIO_WRITE(&sio0, 1); // unlit LED
        } else {
            SIO_WRITE(&sio0, 0); // lit LED
        }
        return muteStatus;
    }else{
        SCI_WRITE(&sci0, "[PLAYER ERR]: toggle_music only allowed by MUSICIAN\n");
        return -1;
    }
}


/* Information */

void print_musicPlayer_verbose(MusicPlayer *self, int unused){
    char musicPlayerInfo[256] = {};
    snprintf(musicPlayerInfo, 256,
             "--------------------MUSICPLAYER--------------------\n"
             "key: %d,  tempo: %d, ",
             self->key, self->tempo);
    SCI_WRITE(&sci0, musicPlayerInfo);
    // print volumn
    SYNC(&toneGenerator, print_volume_info, 0);
    //SCI_WRITE(&sci0, "--------------------MUSICPLAYER--------------------\n");
}