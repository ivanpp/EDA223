#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H

#include "TinyTimber.h"
#include "network.h"

#define KEY_MAX 5
#define KEY_MIN -5
#define KEY_DEFAULT 0
#define TEMPO_MAX 300
#define TEMPO_MIN 30
#define TEMPO_DEFAULT 120
#define BEATMULT_DEFAULT 30000/TEMPO_DEFAULT
#define PERIODS_IDX_DIFF -15
#define BACKUP_DELTA 2

typedef struct{
    Object super;
    int index;
    int key; // offset to the freq index
    int tempo; // bpm: beat per minute
    int beatMult; // ms
    int isStop; // be able to stop
    int hardStopped;
    int ensemble_stop;
    Msg backupMsg; // for backup
    Msg backupMsg2; // for detection
} MusicPlayer;

#define initMusicPlayer() { \
    initObject(), \
    /*index*/ 0, \
    /*key*/ KEY_DEFAULT, \
    /*tempo*/ TEMPO_DEFAULT, \
    /*beatMult*/ BEATMULT_DEFAULT, \
    /*SP: stop*/ 1, \
    /*SP: hardStop*/ 1, \
    /*MP: ensemble_stop*/ 1, \
    NULL, \
} 

extern App app;
extern MusicPlayer musicPlayer;


/* Single-player */
int set_key(MusicPlayer *, int);
int set_tempo(MusicPlayer *, int);
int music_pause_unpause(MusicPlayer *, int);
int music_pause(MusicPlayer *, int);
int music_unpause(MusicPlayer *, int);
int music_stop_start(MusicPlayer *, int);
int music_stop(MusicPlayer *, int);
int music_start(MusicPlayer *, int);
void play_music(MusicPlayer *, int);
/* Multi-player */
void play_index_tone(MusicPlayer *, int);
void play_index_tone_next(MusicPlayer *, int);
void cancel_prev_backup(MusicPlayer *, int);
void play_index_tone_next_backup(MusicPlayer *, int);
void cancel_backup(MusicPlayer *, int);
void sync_LED(MusicPlayer *, int);
void ensemble_ready(MusicPlayer *, int);
void ensemble_stop(MusicPlayer *, int);
void ensemble_start_all(MusicPlayer *, int);
void ensemble_stop_all(MusicPlayer *, int);
void ensemble_restart_all(MusicPlayer *, int);
void play_music_masked(MusicPlayer *, int);
/* MP: key, tempo, mute*/
int set_key_all(MusicPlayer *, int);
int set_tempo_all(MusicPlayer *, int);
void reset_all(MusicPlayer *, int);
int toggle_music(MusicPlayer *, int);
/* Information */
void print_musicPlayer_verbose(MusicPlayer *, int);

#endif