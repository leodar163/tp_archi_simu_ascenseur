#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>

#include "types/user.h"
#include "types/elevator.h"

void *elevatorRoutine(void *arg) {
    ElevatorState *state = (ElevatorState *) arg;

    pthread_mutex_lock(&state->mutex);

    while (state->isRunning) {
        int nearestRequestedFloor = INT_MIN + 2 + FLOOR_NBR;

        printf("l'ascenseur est à l'étage %d\n", state->currentFloor);
        printf("l'ascenseur vérfie s'il on lui a demandé un étage\n");

        // On veut d'abord trouver la requête la plus proche dans la direction actuelle
        // avant de le faire pour tous les étages
        if (state->direction != 0) {
            for (int floor = state->currentFloor; floor < FLOOR_NBR && floor >= 0; floor += state->direction) {
                if (state->requests[floor] == true) {
                    if (abs(floor - state->currentFloor) <= abs(nearestRequestedFloor - state->currentFloor)) {
                        nearestRequestedFloor = floor;
                    }
                }
            }
        }

        // Si on a trouvé aucune requête dans la direction actuelle ou que l'ascenseur est à l'arrêt,
        // on cherche la requête la plus proche parmi tous les étages
        if (nearestRequestedFloor < 0) {
            for (int floor = 0; floor < FLOOR_NBR; floor++) {
                if (state->requests[floor] == true) {
                    if (abs(floor - state->currentFloor) <= abs(nearestRequestedFloor - state->currentFloor)) {
                        nearestRequestedFloor = floor;
                    }
                }
            }
        }

        // Si aucune requête n'a été trouvé, on met l'ascenseur à l'arrêt et on attend/on dort
        if (nearestRequestedFloor < 0) {
            pthread_mutex_unlock(&state->mutex);
            printf("l'ascenseur n'a aucune requête\n");
            state->direction = 0;
            usleep(100000);
            printf("l'ascenseur dort\n");
            pthread_mutex_lock(&state->mutex);
            continue;
        }


        printf("l'ascenseur veut se rendre à l'étage %d\n", nearestRequestedFloor);

        // Si on a trouvé une requête, on vérifie qu'on est pas déjà à l'étage demandé
        // Si oui, on ouvre les portes et on notifie les utilisateurs que l'ascenseur est arrivé à un étage
        if (nearestRequestedFloor == state->currentFloor) {
            printf("l'étage est déjà l'étage actuel\n");
            state->requests[nearestRequestedFloor] = false;

            state->areDoorsOpen = true;
            printf("l'ascenseur ouvre les portes \n");

            pthread_cond_broadcast(&state->onDoorsOpen);

            pthread_mutex_unlock(&state->mutex);
            printf("l'ascenseur attend %fs\n", (float)state->doorOpeningDuration/1000.0f);
            usleep(state->doorOpeningDuration);
            pthread_mutex_lock(&state->mutex);

            state->areDoorsOpen = false;
            printf("l'ascenseur ferme les portes\n");
            continue;
        }

        // Si l'étage trouvé n'est pas l'étage actuel, on  identifie la direction
        // et on se déplace d'un étage dans cette direction
        state->direction = nearestRequestedFloor - state->currentFloor > 0 ? 1 : -1;
        printf(state->direction > 0 ? "l'ascenseur monte d'un étage\n" : "l'ascenseur déscend d'un étage\n");

        usleep(state->movingDuration);
        state->currentFloor = state->currentFloor + state->direction;

        pthread_mutex_unlock(&state->mutex);
    }

    return NULL;
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
            printf("un utilisateur rentre à l'étage %d et demande l'étage %d\n", elevatorState->currentFloor,
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

    return NULL;
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
    pthread_create(&elevatorThread, NULL, elevatorRoutine, &elevatorState);

    // Lecture du fichier users.txt
    FILE *file = fopen("users.txt", "r");
    if (file == NULL) {
        fprintf(stderr, "Erreur: impossible d'ouvrir users.txt\n");
        return 1;
    }

    // Compter le nombre d'utilisateurs
    size_t userNbr = 0;
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        userNbr++;
    }
    rewind(file);

    // Allouer et remplir le tableau d'utilisateurs
    UserState *users = malloc(userNbr * sizeof(UserState));
    pthread_t *userThreads = malloc(userNbr * sizeof(pthread_t));
    bool *instantiatedUsers = malloc(userNbr * sizeof(bool));
    
    for (size_t i = 0; i < userNbr; i++) {
        fgets(line, sizeof(line), file);
        sscanf(line, "%u,%d,%d", &users[i].happenTime, &users[i].startingFloor, &users[i].destinationFloor);
        users[i].isInElevator = false;
        users[i].elevatorState = &elevatorState;
        instantiatedUsers[i] = false;
    }
    fclose(file);

    unsigned int timer = 0;
    size_t userInstantiatedNumber = 0;

    // On parcourt toutes les frames le tableau des utilisateurs pour vérifier s'il faut en instancier un
    while (userInstantiatedNumber < userNbr) {
        for (size_t i = 0; i < userNbr; i++) {
            if (instantiatedUsers[i] == true || users[i].happenTime > timer) continue;

            pthread_t thread;
            printf("un utilisateur arrive à l'étage %d à %fs\n", users[i].startingFloor, (float)timer / 1000.0f);
            pthread_create(&thread, NULL, userRoutine, &users[i]);
            userThreads[i] = thread;
            userInstantiatedNumber++;
            instantiatedUsers[i] = true;
        }
        usleep(100000/60);
        timer += 1000/60;
    }

    for (size_t i = 0; i < userNbr; ++i) {
        pthread_join(userThreads[i], NULL);
    }

    elevatorState.isRunning = false;
    pthread_join(elevatorThread, NULL);
    
    free(users);
    free(userThreads);
    free(instantiatedUsers);
    
    return 0;
}
