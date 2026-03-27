#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include "signal.h"
#include "track.h"
#include "train.h"

// Global variables
Train* global_trains = NULL;
int global_train_count = 0;
FILE* log_file = NULL;
volatile int simulation_running = 1;

long global_arrival_counter = 0;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;
int track_status[TOTAL_TRACKS] = {0};


void log_train(const char* msg, Train* t) {
    pthread_mutex_lock(&print_mutex);
    time_t now = time(NULL);
    char buf[20];
    strftime(buf, sizeof(buf), "%H:%M:%S", localtime(&now));
    printf("[%s] Train %d Loop %d: %s\n", buf, t->id, t->loop_count + 1, msg);
    if(log_file) {
        fprintf(log_file, "[%s] Train %d: %s\n", buf, t->id, msg);
        fflush(log_file);
    }
    pthread_mutex_unlock(&print_mutex);
}

// Deadlock Prevention: Lock all tracks in the path in ascending order
void lock_route(int start, int end) {
    int low = (start < end) ? start : end;
    int high = (start > end) ? start : end;
    for (int i = low - 1; i <= high - 1; i++) {
        pthread_mutex_lock(&tracks[i].mutex);
    }
}

void unlock_route(int start, int end) {
    int low = (start < end) ? start : end;
    int high = (start > end) ? start : end;
    for (int i = low - 1; i <= high - 1; i++) {
        pthread_mutex_unlock(&tracks[i].mutex);
    }
}

// Check if ANY track in the intended path is currently occupied
int is_path_blocked(int start, int end) {
    int low = (start < end) ? start : end;
    int high = (start > end) ? start : end;
    for (int i = low - 1; i <= high - 1; i++) {
        if (track_status[i] != 0) return 1;
    }
    return 0;
}

void* train(void* arg) {
    Train* t = (Train*)arg;
    t->track1 = t->id;
    
    while(t->loop_count < t->max_loops && simulation_running) {
        t->reached_bottom = 0;
        
        // 1. Assign destination
        int direction = (rand() % 2 == 0) ? 1 : -1;
        int max_jump = 1 + (rand() % 2); 
        t->track2 = t->track1 + (direction * max_jump);
        if(t->track2 < 1) t->track2 = 1;
        if(t->track2 > TOTAL_TRACKS) t->track2 = TOTAL_TRACKS;
        if(t->track1 == t->track2) t->track2 = (t->track1 == TOTAL_TRACKS) ? t->track1 - 1 : t->track1 + 1;

        snprintf(t->current_route_str, sizeof(t->current_route_str), "T%d -> T%d", t->track1, t->track2);
        
        t->state = WAITING_SIGNAL;
        pthread_mutex_lock(&queue_mutex);
        t->arrival_time = global_arrival_counter++;
        pthread_mutex_unlock(&queue_mutex);

        // Pre-departure delay
        sleep(t->loop_count == 0 ? t->departure_time : 2);
        
        // 2. The "Anti-Starvation" Loop
        int has_resources = 0;
        while(!has_resources && simulation_running) {
            // Check if it's my turn in the FIFO queue
            pthread_mutex_lock(&queue_mutex);
            Train* oldest = NULL;
            long oldest_time = __LONG_MAX__;
            for(int i = 0; i < global_train_count; i++) {
                if((global_trains[i].state == WAITING_SIGNAL || global_trains[i].state == WAITING_TRACK) 
                   && global_trains[i].arrival_time < oldest_time) {
                    oldest_time = global_trains[i].arrival_time;
                    oldest = &global_trains[i];
                }
            }
            
            if(oldest != NULL && oldest->id == t->id) {
                // It is my turn! Now check if the tracks are actually clear.
                pthread_mutex_lock(&print_mutex);
                if (!is_path_blocked(t->track1, t->track2)) {
                    // PATH IS CLEAR: Claim it immediately
                    track_status[t->track1-1] = t->id;
                    track_status[t->track2-1] = t->id;
                    has_resources = 1;
                    t->state = WAITING_TRACK;
                }
                pthread_mutex_unlock(&print_mutex);
            }
            pthread_mutex_unlock(&queue_mutex);

            if(!has_resources) usleep(100000); // Wait and try again
        }

        if(!simulation_running) break;

        // 3. Acquire Physical Locks and Move
        requestSignal();
        lock_route(t->track1, t->track2);
        
        t->state = MOVING;
        log_train("Departing", t);

        // Wait for GUI to signal completion
        while(!t->reached_bottom && simulation_running) {
            usleep(50000);
        }

        // 4. Release everything
        pthread_mutex_lock(&print_mutex);
        track_status[t->track1-1] = 0;
        track_status[t->track2-1] = 0;
        pthread_mutex_unlock(&print_mutex);

        unlock_route(t->track1, t->track2);
        releaseSignal();

        t->track1 = t->track2;
        t->loop_count++;
        log_train("Arrived", t);
    }

    t->state = FINISHED;
    return NULL;
}
