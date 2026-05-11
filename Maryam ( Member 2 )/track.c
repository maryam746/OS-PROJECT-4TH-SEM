#include "track.h"

TrackSegment tracks[TOTAL_TRACKS];  //originial def yahaan pr baaki sari files will access using extern

void initializeTracks() {
    for(int i = 0; i < TOTAL_TRACKS; i++) {
        tracks[i].id = i;
        pthread_mutex_init(&tracks[i].mutex, NULL);
    }
}
