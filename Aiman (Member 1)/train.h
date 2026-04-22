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
    ARRIVING,
    FINISHED
} TrainState;

typedef struct {
    int id;
    int track1;
    int track2;
    int color_index;
    TrainState state;
    int departure_time;
    int loop_count;
    int max_loops;
    time_t arrival_time;
    time_t finish_time;
    char current_route_str[64];
    volatile int reached_bottom;
    volatile int waiting_at_bottom;
    double total_wait_time;
    int wait_count;
    time_t current_wait_start;
    int is_currently_waiting;
} Train;

void* train(void* arg);

// Global variables
extern Train* global_trains;
extern int global_train_count;
extern FILE* log_file;
extern volatile int simulation_running;

#endif
