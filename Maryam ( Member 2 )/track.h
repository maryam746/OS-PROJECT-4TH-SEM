#ifndef TRACK_H
#define TRACK_H

#include <pthread.h>

#define TOTAL_TRACKS 5

typedef struct {                //har track keliye
    int id;
    pthread_mutex_t mutex;   //one train per track at a time
} TrackSegment;

extern TrackSegment tracks[TOTAL_TRACKS];       //taake multiple definitions na hon aik hi baar consider hojaye

void initializeTracks();    //initializing mutexes

#endif
