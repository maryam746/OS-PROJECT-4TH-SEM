#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include "signals.h"
#include "track.h"
#include "train.h"

Train* global_trains = NULL;
int global_train_count = 0;
FILE* log_file = NULL;
volatile int simulation_running = 1;

long global_arrival_counter = 0;        //for FIFO in waiting
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;        //aik waqt mein sirf aik thread gobal_arrival_counter ko access kr skti hai
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;        //for printing on console and file
pthread_mutex_t track_status_mutex = PTHREAD_MUTEX_INITIALIZER;     //track_status keliye
pthread_mutex_t train_route = PTHREAD_MUTEX_INITIALIZER;

int track_status[TOTAL_TRACKS] = {0};       //track pr konsi train hai


void log_train(const char* msg, Train* t) {
    pthread_mutex_lock(&print_mutex);
    time_t now = time(NULL);
    char buf[20];
    strftime(buf, sizeof(buf), "%H:%M:%S", localtime(&now));
    printf("[%s] Train %d Loop %d: %s\n", buf, t->id, t->loop_count + 1, msg);      //[timestamp] Train 1 Loop 1: Departing
    if(log_file) {
        fprintf(log_file, "[%s] Train %d: %s\n", buf, t->id, msg);
        fflush(log_file);       //forcefully save - buffer mein na bache
    }
    pthread_mutex_unlock(&print_mutex);
}

void lock_route(int start, int end) {           //train jo bhi tracks pr chalegi use lock krdo
    int low = (start < end) ? start : end;
    int high = (start > end) ? start : end;
    for (int i = low - 1; i <= high - 1; i++) {     //asc isliye taake deadlock na ho if 2 trains moving in opp direction
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


static int is_path_blocked(int start, int end) {    //path mein se koi bhi track agar pehle se occupied hai to 1 else 0
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
        pthread_mutex_lock(&train_route);
        int direction = (rand() % 2 == 0) ? 1 : -1;     //right ya left
        int max_jump = 1 + (rand() % 2);        //train 1-2 tracks door jaa skti hai
        t->track2 = t->track1 + (direction * max_jump);
        if(t->track2 < 1) t->track2 = 1;
        if(t->track2 > TOTAL_TRACKS) t->track2 = TOTAL_TRACKS;
        pthread_mutex_unlock(&train_route);

        snprintf(t->current_route_str, sizeof(t->current_route_str), "T%d -> T%d", t->track1, t->track2);
        
        t->state = WAITING_SIGNAL;      
        pthread_mutex_lock(&queue_mutex);
        t->arrival_time = global_arrival_counter++;
        pthread_mutex_unlock(&queue_mutex);

        sleep(t->loop_count == 0 ? t->departure_time : 2);     //pre-departure delay wrna 2 sec sleep
        

        int has_resources = 0;      //signal+track flag

        t->current_wait_start = time(NULL);
        t->is_currently_waiting = 1;
        
        while(!has_resources && simulation_running) {
            pthread_mutex_lock(&queue_mutex);   //check kro queue mein iski turn aayi kia
            Train* oldest = NULL;
            long oldest_time = __LONG_MAX__;
            for(int i = 0; i < global_train_count; i++) {
                if((global_trains[i].state == WAITING_SIGNAL || global_trains[i].state == WAITING_TRACK) && global_trains[i].arrival_time < oldest_time) {
                    oldest_time = global_trains[i].arrival_time;    //arrival order k hisaab se queue
                    oldest = &global_trains[i];
                }
            }
            
            if(oldest != NULL && oldest->id == t->id) {     //agar yehi sb se oldest train hai 
                pthread_mutex_lock(&track_status_mutex);
                if (!is_path_blocked(t->track1, t->track2)) {

                    int low = (t->track1 < t->track2)? t->track1: t->track2;
                    int high = (t->track1 > t->track2)? t->track1 : t->track2;
                    for(int k = low - 1; k <= high - 1; k++) {track_status[k] = t->id;}

                    has_resources = 1;           
                    t->state = WAITING_TRACK;
                }
                pthread_mutex_unlock(&track_status_mutex);
            }
            pthread_mutex_unlock(&queue_mutex);

            if(!has_resources) usleep(100000);  //agar meri baari nhi hai ya blocked hai to 0.1 sec baad retry
        }
        

        if(t->is_currently_waiting) {       //wait khatam - avg wait cal
            time_t wait_end = time(NULL);
            double wait_time = difftime(wait_end, t->current_wait_start);
            t->total_wait_time += wait_time;
            t->wait_count++;        //train ne kitni baar wait kia
            t->is_currently_waiting = 0;
        }

        if(!simulation_running) break;

        requestSignal();
        lock_route(t->track1, t->track2);
        
        t->state = MOVING;
        log_train("Departing", t);

        while(!t->reached_bottom && simulation_running) {   //ab gui train ko neeche tk move krwayega
            usleep(50000);      //avoiding busy waiting
        }
        
        pthread_mutex_lock(&track_status_mutex);
        int low = (t->track1 < t->track2) ? t->track1: t->track2;
        int high = (t->track1 > t->track2) ? t->track1 : t->track2;
        for(int k = low - 1; k <= high - 1; k++) {
            if(track_status[k] == t->id) {  //sirf wohi tracks free kro jo hamare hain
                track_status[k] = 0;
            }
        }
        t->state = ARRIVING;
        pthread_mutex_unlock(&track_status_mutex);
        unlock_route(t->track1, t->track2);
        releaseSignal();

        t->track1 = t->track2;
        t->loop_count++;
        log_train("Arrived", t);
    }

    t->state = FINISHED;
    return NULL;
}
