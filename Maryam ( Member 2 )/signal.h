#ifndef SIGNAL_H
#define SIGNAL_H

#include <semaphore.h>

extern sem_t signalSemaphore;

void initializeSignals();
void requestSignal();       //wait
void releaseSignal();       //post

#endif
