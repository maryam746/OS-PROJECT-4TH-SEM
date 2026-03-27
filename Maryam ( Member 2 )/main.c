#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include "track.h"
#include "signal.h"
#include "train.h"
#include "gui.h"

extern FILE* log_file;
extern volatile int simulation_running;

void init_logging() {
    log_file = fopen("railway_log.txt", "w");
    if(log_file) {
        fprintf(log_file, "=== RAILWAY SIGNAL COORDINATION SYSTEM LOG ===\n");
        time_t now = time(NULL);
        fprintf(log_file, "Simulation Started: %s", ctime(&now));
        fprintf(log_file, "=============================================\n\n");
        fflush(log_file);
    }
}

void close_logging() {
    if(log_file) {
        time_t now = time(NULL);
        fprintf(log_file, "\n=== SIMULATION ENDED: %s ===\n", ctime(&now));
        fclose(log_file);
    }
}

void handle_signal(int sig) {
    printf("\n\nShutting down...\n");
    simulation_running = 0;
}

int main() {
    signal(SIGINT, handle_signal);
    srand(time(NULL));
    
    printf("\n========================================\n");
    printf("RAILWAY SIGNAL COORDINATION SYSTEM\n");
    printf("========================================\n\n");
    
    init_logging();
    initializeTracks();
    initializeSignals();
    
    printf("System initialized with %d tracks\n", TOTAL_TRACKS);
    printf("Signal system: 4 signals available\n");
    printf("Queue system: FIFO (First In First Out)\n\n");
    
    pthread_t gui_thread;
    pthread_create(&gui_thread, NULL, startGUI, NULL);
    sleep(2);
    
    int num_trains = 5;
    pthread_t threads[num_trains];
    Train trains[num_trains];
    
    global_trains = trains;
    global_train_count = num_trains;
    
    printf("Starting %d trains with 3 loops each...\n\n", num_trains);
    
    for(int i = 0; i < num_trains; i++) {
        trains[i].id = i + 1;
        trains[i].color_index = i;
        trains[i].state = WAITING_SIGNAL;
        trains[i].departure_time = i;
        trains[i].loop_count = 0;
        trains[i].max_loops = 3;
        trains[i].arrival_time = 0;
        trains[i].finish_time = 0;
        trains[i].track1 = i + 1;
        trains[i].track2 = 0;
        trains[i].reached_bottom = 0;
        strcpy(trains[i].current_route_str, "");
        
        printf("Train %d: starts on Track %d, %d loops\n", 
               trains[i].id, trains[i].track1, trains[i].max_loops);
        
        pthread_create(&threads[i], NULL, train, &trains[i]);
    }
    
    printf("\n--- SIMULATION RUNNING ---\n");
    printf("Close the GUI window to stop.\n\n");
    
    for(int i = 0; i < num_trains; i++) {
        pthread_join(threads[i], NULL);
        printf("Train %d completed all %d loops\n", i + 1, trains[i].max_loops);
    }
    
    printf("\n========================================\n");
    printf("SIMULATION COMPLETED\n");
    printf("========================================\n");
    
    close_logging();
    sleep(2);
    
    return 0;
}
