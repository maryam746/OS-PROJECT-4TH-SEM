    #include <stdio.h>
#include <pthread.h>
#include "track.h"
#include "signal.h"
#include "train.h"

int main()
{
    initializeTracks();
    initializeSignals();

    pthread_t trains[5];
    int ids[5];

    for(int i = 0; i < 5; i++) {
        ids[i] = i + 1;
        pthread_create(&trains[i], NULL, train, &ids[i]);
    }

    for(int i = 0; i < 5; i++) {
        pthread_join(trains[i], NULL);
    }

    return 0;
}#include <stdio.h>
#include <pthread.h>
#include "track.h"
#include "signal.h"
#include "train.h"

int main()
{
    initializeTracks();
    initializeSignals();

    pthread_t trains[5];
    int ids[5];

    for(int i = 0; i < 5; i++) {
        ids[i] = i + 1;
        pthread_create(&trains[i], NULL, train, &ids[i]);
    }

    for(int i = 0; i < 5; i++) {
        pthread_join(trains[i], NULL);
    }

    return 0;
}#include <stdio.h>
#include <pthread.h>
#include "track.h"
#include "signal.h"
#include "train.h"

int main()
{
    initializeTracks();
    initializeSignals();

    pthread_t trains[5];
    int ids[5];

    for(int i = 0; i < 5; i++) {
        ids[i] = i + 1;
        pthread_create(&trains[i], NULL, train, &ids[i]);
    }

    for(int i = 0; i < 5; i++) {
        pthread_join(trains[i], NULL);
    }

    return 0;
}
