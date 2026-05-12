#include "signals.h"

sem_t signalSemaphore;

void initializeSignals() {
    sem_init(&signalSemaphore, 0, 4);   //0 for threads, max 4 trains running at a time congested na ho isliye
}

void requestSignal() {
    sem_wait(&signalSemaphore);
}

void releaseSignal() {
    sem_post(&signalSemaphore);
}
