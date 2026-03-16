#ifndef SIGNAL_H
#define SIGNAL_H

#include <semaphore.h>

extern sem_t signalSemaphore;  // Added this line

void initializeSignals();
void requestSignal();
void releaseSignal();

#endif
