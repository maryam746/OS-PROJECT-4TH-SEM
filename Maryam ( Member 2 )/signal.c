#include "signal.h"

sem_t signalSemaphore;

void initializeSignals() {
    sem_init(&signalSemaphore, 0, 4);
}

void requestSignal() {
    sem_wait(&signalSemaphore);
}

void releaseSignal() {
    sem_post(&signalSemaphore);
}
