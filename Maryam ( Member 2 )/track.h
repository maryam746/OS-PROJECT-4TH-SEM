#ifndef TRACK_H
#define TRACK_H

#include <pthread.h>

#define TOTAL_TRACKS 5

typedef struct {
    int id;
    pthread_mutex_t mutex;
} TrackSegment;

extern TrackSegment tracks[TOTAL_TRACKS];

void initializeTracks();

#endif
