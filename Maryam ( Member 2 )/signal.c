#include "signal.h"

sem_t signalSemaphore;

void initializeSignals() {
    sem_init(&signalSemaphore, 0, 2);
}

void requestSignal() {
    sem_wait(&signalSemaphore);
}

void releaseSignal() {
    sem_post(&signalSemaphore);
}
