//
// Created by leobg on 01/01/2026.
//

#ifndef TP_ARCHI_SIMU_ASCENSEUR_USER_H
#define TP_ARCHI_SIMU_ASCENSEUR_USER_H
typedef struct {
    float happenTime;
    int floor;
    int destinationFloor;
} User;

typedef struct {
    bool isInElevator;
} UserState;

#endif //TP_ARCHI_SIMU_ASCENSEUR_USER_H
