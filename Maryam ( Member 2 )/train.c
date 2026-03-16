#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include "signal.h"
#include "routing.h"
#include "train.h"

// Define total number of tracks
#define TOTAL_TRACKS 5

// ANSI color codes for trains
const char* colors[] = {
    "\x1b[31m", // Red
    "\x1b[32m", // Green
    "\x1b[33m", // Yellow
    "\x1b[34m", // Blue
    "\x1b[35m"  // Magenta
};
#define RESET "\x1b[0m"

void* train(void* arg)
{
    int train_id = *(int*)arg;
    srand(time(NULL) + train_id); // unique seed per train

    const char* color = colors[(train_id-1) % 5]; // assign color

    int track1 = rand() % TOTAL_TRACKS;
    int track2;
    do {
        track2 = rand() % TOTAL_TRACKS;
    } while(track2 == track1);

    printf("%sTrain %d requesting signal%s\n", color, train_id, RESET);

    requestSignal();

    printf("%sTrain %d requesting tracks %d and %d%s\n", color, train_id, track1, track2, RESET);

    acquireTracks(track1, track2);

    int travel_time = rand() % 3 + 1; // 1-3 seconds
    printf("%sTrain %d moving through tracks for %d seconds%s\n", color, train_id, travel_time, RESET);

    sleep(travel_time);

    printf("%sTrain %d leaving tracks%s\n", color, train_id, RESET);

    releaseTracks(track1, track2);
    releaseSignal();

    printf("%sTrain %d finished route%s\n", color, train_id, RESET);

    return NULL;
}
