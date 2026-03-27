#ifndef TRAIN_H
#define TRAIN_H
#include "track.h"
#include <time.h>
#include <stdio.h>

extern int track_status[TOTAL_TRACKS];

typedef enum {
    WAITING_SIGNAL,
    WAITING_TRACK,
    MOVING,
    FINISHED
} TrainState;

typedef struct {
    int id;
    int track1;          // Current track (1-5)
    int track2;          // Destination track (1-5)
    int color_index;
    TrainState state;
    int departure_time;
    int loop_count;
    int max_loops;
    time_t arrival_time;
    time_t finish_time;
    char current_route_str[64];  // Route description for GUI
    volatile int reached_bottom;
} Train;

void* train(void* arg);

// Global variables
extern Train* global_trains;
extern int global_train_count;
extern FILE* log_file;
extern volatile int simulation_running;

#endif
