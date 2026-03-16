#include "track.h"

TrackSegment tracks[NUM_TRACKS];

void initializeTracks() {

    for(int i = 0; i < NUM_TRACKS; i++) {
        tracks[i].id = i;
        pthread_mutex_init(&tracks[i].mutex, NULL);
    }

}
