#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>

#include "types/user.h"
#include "types/elevator.h"

void *elevatorRoutine(void *arg) {
    ElevatorState *state = (ElevatorState *) arg;

    pthread_mutex_lock(&state->mutex);

    while (true) {
        int nearestRequestedFloor = INT_MIN + 2 + FLOOR_NBR;

        printf("on trouve la requête la plus proche\n");
        for (int floor = 0; floor < FLOOR_NBR; floor++) {
            if (state->requests[floor] == true) {
                if (abs(floor - state->currentFloor) <= abs(nearestRequestedFloor - state->currentFloor)) {
                    nearestRequestedFloor = floor;
                }
            }
        }

        if (nearestRequestedFloor < 0) {
            printf("on a pas de requête\n");
            state->direction = 0;
            usleep(1000 / 60);
            printf("on dort\n");
            break;
        }

        if (nearestRequestedFloor == state->currentFloor) {
            printf("la requete la plus proche est l'étage actuel (request: %d, current: %d)\n", nearestRequestedFloor,
                   state->currentFloor);
            state->requests[nearestRequestedFloor] = false;
            printf("on consomme la requête\n");
            state->areDoorsOpen = true;
            printf("on ouvre les portes \n");

            pthread_cond_broadcast(&state->onDoorsOpen);

            pthread_mutex_unlock(&state->mutex);
            printf("on attend\n");
            usleep(state->doorOpeningDuration);
            pthread_mutex_lock(&state->mutex);

            state->areDoorsOpen = false;
            printf("on ferme les portes\n");
            continue;
        }

        printf("la requête la plus proche n'est pas l'étage actuel\n");
        state->direction = nearestRequestedFloor - state->currentFloor > 0 ? 1 : -1;
        printf("on calcule la direction : %d\n", state->direction);
        printf("on va au prochain étage\n");
        usleep(state->movingDuration);
        state->currentFloor = state->currentFloor + 1;

        pthread_mutex_unlock(&state->mutex);
    }
}

void *userRoutine(void *arg) {
    UserState *state = (UserState *) arg;
    ElevatorState *elevatorState = state->elevatorState;

    printf("user before mutex lock\n");

    pthread_mutex_lock(&elevatorState->mutex);

    while (elevatorState->areDoorsOpen && state->destinationFloor != elevatorState->currentFloor) {
        pthread_cond_wait(&elevatorState->onDoorsOpen, &elevatorState->mutex);
    }

    printf("un utilisateur est heureux de descendre à l'étage %d\n", elevatorState->currentFloor);
    pthread_mutex_unlock(&elevatorState->mutex);
}

int main(void) {
    ElevatorState elevatorState = {
        .currentFloor = 0,
        .direction = 0,
        .areDoorsOpen = false,
        .doorOpeningDuration = 2000,
        .movingDuration = 3000,
        .mutex = PTHREAD_MUTEX_INITIALIZER,
        .onDoorsOpen = PTHREAD_COND_INITIALIZER,
        .requests[0] = true,
        .requests[1] = false,
        .requests[2] = false,
        .requests[3] = true,
        .requests[4] = false,
    };

    pthread_t elevatorThread;
    pthread_create(&elevatorThread, nullptr, elevatorRoutine, &elevatorState);

    constexpr unsigned int userNbr = 4;

    UserState users[userNbr] = {
        {
            .happenTime = 0,
            .floor = 4,
            .destinationFloor = 3,
            .isInElevator = false,
            .elevatorState = &elevatorState,
        },
        {
            .happenTime = 1,
            .floor = 2,
            .destinationFloor = 0,
            .isInElevator = false,
            .elevatorState = &elevatorState,
        },
        {
            .happenTime = 1,
            .floor = 2,
            .destinationFloor = 3,
            .isInElevator = false,
            .elevatorState = &elevatorState,
        },
        {
            .happenTime = 1,
            .floor = 2,
            .destinationFloor = 3,
            .isInElevator = false,
            .elevatorState = &elevatorState,
        }
    };

    pthread_t userThreads[userNbr];

    for (int i = 0; i < userNbr; i++) {
        pthread_t thread;
        pthread_create(&thread, nullptr, userRoutine, &users[i]);
        userThreads[i] = thread;
    }

    for (int i = 0; i < userNbr; ++i) {
        pthread_join(userThreads[i], nullptr);
    }

    pthread_join(elevatorThread, nullptr);
    return 0;
}
