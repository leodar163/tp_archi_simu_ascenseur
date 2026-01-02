//
// Created by leobg on 01/01/2026.
//

#include <pthread.h>

#ifndef TP_ARCHI_SIMU_ASCENSEUR_ELEVATOR_H
#define TP_ARCHI_SIMU_ASCENSEUR_ELEVATOR_H
#define FLOOR_NBR 5

typedef struct {
    int currentFloor;
    int direction;
    bool requests[FLOOR_NBR];
    bool areDoorsOpen;
    unsigned int doorOpeningDuration;
    unsigned int movingDuration;

    bool isRunning;

    pthread_mutex_t mutex;
    pthread_cond_t onDoorsOpen;
} ElevatorState;
#endif //TP_ARCHI_SIMU_ASCENSEUR_ELEVATOR_H
