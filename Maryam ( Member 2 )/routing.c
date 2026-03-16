#include "routing.h"
#include "track.h"
#include <pthread.h>

void acquireTracks(int t1, int t2) {

    if(t1 < t2) {
        pthread_mutex_lock(&tracks[t1].mutex);
        pthread_mutex_lock(&tracks[t2].mutex);
    }
    else {
        pthread_mutex_lock(&tracks[t2].mutex);
        pthread_mutex_lock(&tracks[t1].mutex);
    }

}

void releaseTracks(int t1, int t2) {

    pthread_mutex_unlock(&tracks[t1].mutex);
    pthread_mutex_unlock(&tracks[t2].mutex);

}
