#ifndef SIGNAL_H
#define SIGNAL_H

#include <semaphore.h>

void initializeSignals();
void requestSignal();
void releaseSignal();

#endif
