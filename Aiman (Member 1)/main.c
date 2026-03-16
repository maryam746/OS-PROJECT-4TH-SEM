#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include "track.h"
#include "signal.h"
#include "train.h"

int main()
{
    srand(time(NULL));
    printf("Railway Signaling System Simulation Starting...\n");

    initializeTracks();
    initializeSignals();

    int num_trains = 6;
    pthread_t threads[num_trains];
    Train trains[num_trains];

    // Initialize trains with priority and departure schedule
    for(int i=0;i<num_trains;i++){
        trains[i].id = i+1;
        trains[i].color_index = i;
        trains[i].state = WAITING_SIGNAL;
        trains[i].priority = (i%2==0)?1:0; // alternate express/normal
        trains[i].departure_time = i*2;    // stagger departures

        pthread_create(&threads[i],NULL,train,&trains[i]);
    }

    for(int i=0;i<num_trains;i++){
        pthread_join(threads[i],NULL);
    }

    printf("\nAll trains finished.\n");

    // Analytics
    double total_wait=0, total_travel=0;
    printf("\nTRAIN ANALYTICS:\n");
    for(int i=0;i<num_trains;i++){
        double wait_time = difftime(trains[i].track_acquired_time, trains[i].arrival_time);
        double travel_time = difftime(trains[i].finish_time, trains[i].track_acquired_time);
        total_wait += wait_time;
        total_travel += travel_time;
        printf("Train %d [%s]: Wait = %.0f s, Travel = %.0f s\n",
               trains[i].id,
               (trains[i].priority==1)?"Express":"Normal",
               wait_time, travel_time);
    }

    printf("Average Wait Time: %.2f s\n", total_wait/num_trains);
    printf("Average Travel Time: %.2f s\n", total_travel/num_trains);

    // Track utilization summary
    printf("\nTRACK UTILIZATION SUMMARY:\n");
    for(int i=0;i<TOTAL_TRACKS;i++){
        printf("Track %d used %d times\n", i, track_utilization[i]);
    }

    return 0;
}
