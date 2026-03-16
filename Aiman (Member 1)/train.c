#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include "signal.h"
#include "routing.h"
#include "train.h"

// ANSI color codes for trains
const char* colors[] = {"\x1b[31m","\x1b[32m","\x1b[33m","\x1b[34m","\x1b[35m"};
#define RESET "\x1b[0m"

// Thread-safe printing
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

// Track occupancy: 0 = free, train_id = occupied
int track_status[TOTAL_TRACKS] = {0};

// Track utilization counter (extern declared in train.h)
int track_utilization[TOTAL_TRACKS] = {0};

// Priority train management
int waiting_express = 0;
pthread_mutex_t priority_mutex = PTHREAD_MUTEX_INITIALIZER;

// Convert train state to string
const char* stateToString(TrainState s){
    switch(s){
        case WAITING_SIGNAL: return "Waiting for signal";
        case WAITING_TRACK: return "Waiting for track";
        case MOVING: return "Moving";
        case FINISHED: return "Finished";
        default: return "Unknown";
    }
}

// Display track occupancy
void displayTracks()
{
    pthread_mutex_lock(&print_mutex);
    printf("\nTRACK STATUS\n");
    for(int i=0;i<TOTAL_TRACKS;i++){
        if(track_status[i]==0)
            printf("Track %d : Free | Used %d times\n", i, track_utilization[i]);
        else
            printf("Track %d : Train %d | Used %d times\n", i, track_status[i], track_utilization[i]);
    }
    printf("\n");
    pthread_mutex_unlock(&print_mutex);
}

// Thread-safe logging with timestamp
void log_train(const char* msg, Train* t)
{
    pthread_mutex_lock(&print_mutex);
    time_t now = time(NULL);
    char buf[20];
    strftime(buf, sizeof(buf), "%H:%M:%S", localtime(&now));

    printf("[%s]%sTrain %d [%s]: %s%s\n",
           buf,
           colors[t->color_index % 5],
           t->id,
           (t->priority==1)?"Express":"Normal",
           msg,
           RESET);
    pthread_mutex_unlock(&print_mutex);
}

// Train thread function
void* train(void* arg)
{
    Train* t = (Train*)arg;

    // Wait until scheduled departure
    sleep(t->departure_time);

    t->arrival_time = time(NULL);
    t->state = WAITING_SIGNAL;
    log_train("Departed station (scheduled)", t);

    // Random route assignment
    t->track1 = rand() % TOTAL_TRACKS;
    do { t->track2 = rand() % TOTAL_TRACKS; } while(t->track2 == t->track1);
    log_train("Route assigned", t);

    // Express priority at signal
    if(t->priority == 1){
        pthread_mutex_lock(&priority_mutex);
        waiting_express++;
        pthread_mutex_unlock(&priority_mutex);
    }

    log_train("Waiting for signal", t);
    int acquired = 0;
    while(!acquired){
        pthread_mutex_lock(&priority_mutex);
        if(t->priority==1 || waiting_express==0){
            sem_wait(&signalSemaphore); // signal acquisition
            acquired = 1;
        }
        pthread_mutex_unlock(&priority_mutex);
        if(!acquired) sleep(1);
    }

    if(t->priority==1){
        pthread_mutex_lock(&priority_mutex);
        waiting_express--;
        pthread_mutex_unlock(&priority_mutex);
    }

    t->signal_acquired_time = time(NULL);

    // Track acquisition
    t->state = WAITING_TRACK;
    char msg[50];
    snprintf(msg,sizeof(msg),"Requesting tracks %d and %d",t->track1,t->track2);
    log_train(msg,t);

    acquireTracks(t->track1,t->track2);
    t->track_acquired_time = time(NULL);

    // Occupy tracks & increment utilization
    track_status[t->track1] = t->id;
    track_status[t->track2] = t->id;
    track_utilization[t->track1]++;
    track_utilization[t->track2]++;
    displayTracks();

    // Move train
    t->state = MOVING;
    int travel_time = rand()%3 + 1;
    snprintf(msg,sizeof(msg),"Moving for %d seconds", travel_time);
    log_train(msg,t);
    sleep(travel_time);

    // Release tracks
    log_train("Leaving tracks",t);
    track_status[t->track1]=0;
    track_status[t->track2]=0;
    displayTracks();

    releaseTracks(t->track1,t->track2);
    releaseSignal();

    t->state = FINISHED;
    t->finish_time = time(NULL);
    log_train("Finished route",t);

    return NULL;
}
