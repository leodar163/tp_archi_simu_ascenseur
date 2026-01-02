cmake_minimum_required(VERSION 3.28)
project(tp_archi_simu_ascenseur C)

set(CMAKE_C_STANDARD 23)

add_executable(tp_archi_simu_ascenseur main.c
        types/user.h
)
