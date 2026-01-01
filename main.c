#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>

#include "types/user.h"
#include "types/elevator.h"

void* userRoutine(void* arg) {
    User* user = (User*)arg;
    UserState* state = malloc(sizeof(UserState));

    while (true) {
        break;
    }

    free(state);
}

void* elevatorRoutine(void* arg) {
    ElevatorState* state = (ElevatorState*)arg;

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
            sleep(1/60);
            printf("on dort\n");
            break;
        }

        if (nearestRequestedFloor == state->currentFloor) {
            printf("la requete la plus proche est l'étage actuel (request: %d, current: %d)\n", nearestRequestedFloor, state->currentFloor);
            state->requests[nearestRequestedFloor] = false;
            printf("on consomme la requête\n");
            state->areDoorsOpen = true;
            printf("on ouvre les portes \n");

            printf("on attend\n");
            sleep(state->doorOpeningDuration);

            state->areDoorsOpen = false;
            printf("on ferme les portes\n");
            continue;
        }

        printf("la requête la plus proche n'est pas l'étage actuel\n");
        state->direction = nearestRequestedFloor - state->currentFloor > 0 ? 1 : -1;
        printf("on calcule la direction : %d\n", state->direction);
        printf("on va au prochain étage\n");
        sleep(state->movingDuration);
        state->currentFloor = state->currentFloor + 1;
    }
}

int main(void) {
    ElevatorState elevatorState = {
        .currentFloor = 0,
        .direction = 0,
        .areDoorsOpen = false,
        .doorOpeningDuration = 3,
        .movingDuration = 5,
        .requests[0] = true,
        .requests[1] = false,
        .requests[2] = false,
        .requests[3] = true,
        .requests[4] = false,
    };

    pthread_t elevatorThread;
    pthread_create(&elevatorThread, NULL, elevatorRoutine, &elevatorState);

    pthread_join(elevatorThread, NULL);

    User users[] = {
        {
            .happenTime = 0,
            .floor = 4,
            .destinationFloor = 1
        },
        {
            .happenTime = 1,
            .floor = 2,
            .destinationFloor = 0
        }
    };

    // for (int i = 0; i < sizeof(users); i++) {
    //     pthread_t thread;
    //     pthread_create(&thread, nullptr, userRoutine, &users[i]);
    // }

    return 0;
}
