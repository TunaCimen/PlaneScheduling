// Created by Tuna Çimen on 29.05.2024.
//
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "thread_sleep.h"


#define TRUE 1
#define FALSE 0
#define NUMBER_OF_AIRPLANES 10
#define LANDING 0
#define TAKEOFF 1

pthread_mutex_t lock_rway;
pthread_mutex_t comm_lock;
pthread_mutex_t time_lock;
pthread_mutex_t log_lock;
pthread_mutex_t starve_lock;
pthread_mutex_t land_count_lock;
int favored_land;
int time_passed;
float probability;
int runway;
int simulation_time;

int ready_to_land_arr[NUMBER_OF_AIRPLANES];
int landed_arr[NUMBER_OF_AIRPLANES];
int ready_to_take_off[NUMBER_OF_AIRPLANES];

typedef struct {
    int status;
    int id; // ID for plane.
    pthread_t plane_id; // thread id for the plane
} airplane;

typedef struct {
    int plane_id;
    char status;
    int request_time;
    int runway_time;
    int turnaround_time;
} log_entry;

log_entry log_entries[NUMBER_OF_AIRPLANES * 2];
int log_index = 0;

static void add_log_entry(int id, char status, int request_time, int runway_time, int turnaround_time) {
    pthread_mutex_lock(&log_lock);
    log_entries[log_index].plane_id = id;
    log_entries[log_index].status = status;
    log_entries[log_index].request_time = request_time;
    log_entries[log_index].runway_time = runway_time;
    log_entries[log_index].turnaround_time = turnaround_time;
    log_index++;
    pthread_mutex_unlock(&log_lock);
}

static void print_log() {
    printf("PlaneID Status Request Time Runway Time Turnaround Time\n");
    for (int i = 0; i < log_index; i++) {
        printf("%d      %c      %d            %d          %d\n",
               log_entries[i].plane_id,
               log_entries[i].status,
               log_entries[i].request_time,
               log_entries[i].runway_time,
               log_entries[i].turnaround_time);
    }
}

static void land_plane(int id) {
    pthread_mutex_lock(&lock_rway);
    pthread_mutex_lock(&land_count_lock);

    favored_land++;
    pthread_mutex_unlock(&land_count_lock);

    if (time_passed >= simulation_time) {
        pthread_mutex_unlock(&lock_rway);
        return;
    }

    pthread_sleep(2);
    int landing_time = time_passed;
    printf("Plane %d has landed. Time: %d\n", id, landing_time);

    landed_arr[id] = 1;
    int request_time = ready_to_land_arr[id];
    ready_to_land_arr[id] = -5;
    add_log_entry(id, 'L',  request_time, landing_time, landing_time-request_time);
    if(favored_land >= 3){
        pthread_mutex_unlock(&starve_lock);
    }
    pthread_mutex_unlock(&land_count_lock);

    pthread_mutex_unlock(&lock_rway);



}

static void request_landing(int id) {
    pthread_mutex_lock(&comm_lock);
    if (time_passed >= simulation_time) {
        pthread_mutex_unlock(&comm_lock);
        return;
    }
    int request_time = time_passed;
    printf("Plane %d wants to land. Time: %d\n", id, request_time);
    ready_to_land_arr[id] = time_passed;
    pthread_mutex_unlock(&comm_lock);
    land_plane(id);
}

static void fly_plane(int id) {
    pthread_mutex_lock(&starve_lock);
    pthread_mutex_lock(&lock_rway);
    if (time_passed >= simulation_time) {
        pthread_mutex_unlock(&lock_rway);
        return;
    }
    int request_time = ready_to_take_off[id];
    ready_to_take_off[id] = -5;
    pthread_sleep(2);
    int takeoff_time = time_passed;
    printf("Plane %d has taken off. Time: %d\n", id, takeoff_time);
    landed_arr[id] = 0;
    add_log_entry(id, 'D', request_time, takeoff_time, takeoff_time-request_time);
    pthread_mutex_unlock(&lock_rway);
    pthread_mutex_lock(&land_count_lock);
    favored_land=0;
    pthread_mutex_unlock(&land_count_lock);
}

static void request_takeoff(int id) {
    pthread_mutex_lock(&comm_lock);
    if (time_passed >= simulation_time) {
        pthread_mutex_unlock(&comm_lock);
        return;
    }
    int request_time = time_passed;
    printf("Plane %d wants to take off. Time: %d\n", id, request_time);
    ready_to_take_off[id] = time_passed;
    pthread_mutex_unlock(&comm_lock);
    fly_plane(id);
}

static void* airplane_control(void* args) {
    airplane* plane = (airplane*)args;
    while (TRUE) {
        if (time_passed >= simulation_time) {
            break;
        }

        float random_value = (float)rand()/RAND_MAX;

        pthread_sleep(1);
        if(time_passed > ready_to_take_off[plane->id]+15 && ready_to_take_off[plane->id] != -5  ){
            pthread_mutex_unlock(&starve_lock);
        }

        if (random_value < probability && ready_to_land_arr[plane->id] == -5 && landed_arr[plane->id] == 0) {
            request_landing(plane->id);
        }
        if (random_value>= probability && landed_arr[plane->id] == 1 && ready_to_take_off[plane->id] == -5) {
            request_takeoff(plane->id);
        }

    }
    return NULL;
}

static void* run_tower_main(void* args) {
    int passed_sec = 0;
    while (passed_sec < simulation_time) {
        pthread_sleep(1);
        time_passed = ++passed_sec;
    }
    return NULL;
}

int main(int argc, char *argv[]) {

    for(int i = 0; i<NUMBER_OF_AIRPLANES ; i++){
        ready_to_land_arr[i] = -5;
    }
    for(int i = 0; i<NUMBER_OF_AIRPLANES ; i++){
        ready_to_take_off[i] = -5;
    }

    if (argc != 3) {
        printf("Usage: %s <simulation_time> <probability>\n", argv[0]);
        return 1;
    }

    simulation_time = atoi(argv[1]);
    printf("%d\n",simulation_time);
    probability = atof(argv[2]);
    if(probability > 1 || probability < 0){
	printf("Probability must be between 0 and 1.!!./n");
        return 1;
	}
    printf("%f\n", probability);
    if (simulation_time <= 0) {
        printf("Simulation time must be a positive integer.\n");
        return 1;
    }

    time_passed = 0;
    favored_land = 0;
    airplane* planes;
    pthread_t tower_main;
    pthread_create(&tower_main, NULL, run_tower_main, NULL);

    //init mutexes;
    if (pthread_mutex_init(&comm_lock, NULL) != 0) {
        printf("Mutex init fail\n");
    }
    if(pthread_mutex_init(&starve_lock, NULL) != 0){
        printf("Mutex init fail\n");
    }
    if(pthread_mutex_init(&land_count_lock, NULL) != 0){
        printf("Mutex init fail\n");
    }
    if (pthread_mutex_init(&lock_rway, NULL) != 0) {
        printf("Mutex init fail\n");
    }
    if (pthread_mutex_init(&time_lock, NULL) != 0) {
        printf("Mutex init fail\n");
    }
    if (pthread_mutex_init(&log_lock, NULL) != 0) {
        printf("Mutex init fail\n");
    }

    //create the planes.
    planes = (airplane*) calloc(NUMBER_OF_AIRPLANES, sizeof(airplane));
    if (!planes) {
        perror("Error in memory allocation for planes.");
        exit(1);
    }

    int status;
    for (int i = 0; i < NUMBER_OF_AIRPLANES; i++) {
        planes[i].id = i + 1;
        status = pthread_create(&planes[i].plane_id, NULL, airplane_control, (void*)&planes[i]);
        if (status != 0) {
            printf("Error in function pthread_create!!\n");
        }
    }

    // Wait for all threads to finish
    pthread_join(tower_main, NULL);
    for (int i = 0; i < NUMBER_OF_AIRPLANES; i++) {
        pthread_join(planes[i].plane_id, NULL);
    }

    // Print the log
    print_log();

    // Destroy mutexes
    pthread_mutex_destroy(&comm_lock);
    pthread_mutex_destroy(&lock_rway);
    pthread_mutex_destroy(&time_lock);
    pthread_mutex_destroy(&log_lock);

    free(planes);
    return 0;
}
