//
// Created by leobg on 01/01/2026.
//

#include <pthread.h>

#include "elevator.h"

#ifndef TP_ARCHI_SIMU_ASCENSEUR_USER_H
#define TP_ARCHI_SIMU_ASCENSEUR_USER_H
typedef struct {
    float happenTime;
    int startingFloor;
    int destinationFloor;
    bool isInElevator;

    ElevatorState* elevatorState;
} UserState;

#endif //TP_ARCHI_SIMU_ASCENSEUR_USER_H
