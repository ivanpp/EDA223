#ifndef PLAYLIST_H
#define PLAYLIST_H

typedef struct {
    /* a playlist of several songs */
    int length;
} PlayList;

typedef struct {
    char title[16];
    int notes[32];
    int tempos[32];
} Song;

const int brotherJohnNotes[32] = {0, 2, 4, 0,
                                  0, 2, 4, 0,
                                  4, 5, 7,
                                  4, 5, 7,
                                  7, 9, 7, 5, 4, 0,
                                  7, 9, 7, 5, 4, 0,
                                  0, -5, 0,
                                  0, -5, 0};

const int brotherJohnTempos[32] = {2, 2, 2, 2, 
                                   2, 2, 2, 2,
                                   2, 2, 4, 
                                   2, 2, 4,
                                   1, 1, 1, 1, 2, 2,
                                   1, 1, 1, 1, 2, 2, 
                                   2, 2, 4, 
                                   2, 2, 4};

extern Song brotherJohn;

#define PERIODS_IDX_DIFF -15
extern const int pianoPeriods[32];

#endif