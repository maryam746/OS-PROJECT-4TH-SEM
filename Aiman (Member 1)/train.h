#ifndef TRAIN_H
#define TRAIN_H

#include <time.h>

#define TOTAL_TRACKS 5

typedef enum {
    WAITING_SIGNAL,
    WAITING_TRACK,
    MOVING,
    FINISHED
} TrainState;

typedef struct {
    int id;
    int track1;
    int track2;
    int color_index;
    TrainState state;

    int priority;          // 0 = normal, 1 = express
    int departure_time;    // scheduled departure (seconds from start)

    // Timestamp fields for analytics
    time_t arrival_time;
    time_t signal_acquired_time;
    time_t track_acquired_time;
    time_t finish_time;

} Train;

void* train(void* arg);

// Make track utilization accessible to main.c
extern int track_utilization[TOTAL_TRACKS];

#endif
