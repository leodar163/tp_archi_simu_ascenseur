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

    while (state->isRunning) {
        int nearestRequestedFloor = INT_MIN + 2 + FLOOR_NBR;

        printf("on trouve la requête la plus proche\n");

        if (state->direction != 0) {
            for (int floor = state->currentFloor; floor < FLOOR_NBR && floor >= 0; floor += state->direction) {
                if (state->requests[floor] == true) {
                    if (abs(floor - state->currentFloor) <= abs(nearestRequestedFloor - state->currentFloor)) {
                        nearestRequestedFloor = floor;
                    }
                }
            }
        }

        if (nearestRequestedFloor < 0) {
            for (int floor = 0; floor < FLOOR_NBR; floor++) {
                if (state->requests[floor] == true) {
                    if (abs(floor - state->currentFloor) <= abs(nearestRequestedFloor - state->currentFloor)) {
                        nearestRequestedFloor = floor;
                    }
                }
            }
        }

        if (nearestRequestedFloor < 0) {
            printf("on a pas de requête\n");
            state->direction = 0;
            usleep(1000);
            printf("on dort\n");
            continue;
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
        state->currentFloor = state->currentFloor + state->direction;

        pthread_mutex_unlock(&state->mutex);
    }
}

void *userRoutine(void *arg) {
    UserState *state = (UserState *) arg;
    ElevatorState *elevatorState = state->elevatorState;

    printf("un utilisateur appelle l'ascenseur à l'étage %d\n", state->startingFloor);
    elevatorState->requests[state->startingFloor] = true;

    pthread_mutex_lock(&elevatorState->mutex);
    while (!state->isInElevator) {
        pthread_cond_wait(&elevatorState->onDoorsOpen, &elevatorState->mutex);

        if (elevatorState->areDoorsOpen && elevatorState->currentFloor == state->startingFloor) {
            printf("un utilisateur monte à l'étage %d et demande l'étage %d\n", elevatorState->currentFloor,
                   state->destinationFloor);
            elevatorState->requests[state->destinationFloor] = true;
            state->isInElevator = true;
        }
    }
    pthread_mutex_unlock(&elevatorState->mutex);

    pthread_mutex_lock(&elevatorState->mutex);
    while (state->isInElevator) {
        pthread_cond_wait(&elevatorState->onDoorsOpen, &elevatorState->mutex);

        if (elevatorState->areDoorsOpen && elevatorState->currentFloor == state->destinationFloor) {
            printf("un utilisateur descend à l'étage %d\n", elevatorState->currentFloor);
            state->isInElevator = false;
        }
    }
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
        .isRunning = true,
        .requests = {false},
    };

    pthread_t elevatorThread;
    pthread_create(&elevatorThread, nullptr, elevatorRoutine, &elevatorState);

    constexpr unsigned int userNbr = 2;

    UserState users[userNbr] = {
        {
            .happenTime = 0,
            .startingFloor = 4,
            .destinationFloor = 3,
            .isInElevator = false,
            .elevatorState = &elevatorState,
        },
        {
            .happenTime = 1,
            .startingFloor = 2,
            .destinationFloor = 1,
            .isInElevator = false,
            .elevatorState = &elevatorState,
        },
        // {
        //     .happenTime = 1,
        //     .floor = 2,
        //     .startingFloor = 3,
        //     .isInElevator = false,
        //     .elevatorState = &elevatorState,
        // },
        // {
        //     .happenTime = 1,
        //     .floor = 2,
        //     .startingFloor = 3,
        //     .isInElevator = false,
        //     .elevatorState = &elevatorState,
        // }
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
