#ifndef TRAIN_H
#define TRAIN_H
#include "track.h"
#include <time.h>
#include <stdio.h>

extern int track_status[TOTAL_TRACKS];  //0=free wrna train_id 1-5

typedef enum {
    WAITING_SIGNAL,     //chosen destination but waiting for sem
    WAITING_TRACK,      //signal mil gya dest track keliye wait kr rhe hain
    MOVING,
    ARRIVING,
    FINISHED
} TrainState;

typedef struct {
    int id;
    int track1;             //start
    int track2;             //end
    int color_index;
    TrainState state;
    int departure_time;     //shru ka delay
    int loop_count;
    int max_loops;          //3
    time_t arrival_time;        //for queueing at the top
    time_t finish_time;
    char current_route_str[64];         //string jo show hogi
    volatile int reached_bottom;    //gui thread train thread ko bataata hai to complete loop iteration
    double total_wait_time;
    int wait_count;
    time_t current_wait_start;
    int is_currently_waiting;
} Train;

void* train(void* arg);

extern Train* global_trains;
extern int global_train_count;      //no of active trains
extern FILE* log_file;
extern volatile int simulation_running;

#endif
