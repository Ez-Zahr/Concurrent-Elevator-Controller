#include "hw6.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>

struct psg {
    int id;
    int from_floor;
    int to_floor;
    int elenumber;
    enum { WAITING = 1,
           RIDING = 2,
           DONE = 3 } state;
    struct psg* next;
    pthread_cond_t cond;
} passengers[PASSENGERS];

pthread_mutex_t request_lock;

struct ele {
    int current_floor;
    int direction;
    int occupancy;
    int requests;
    enum { ELEVATOR_ARRIVED = 1,
           ELEVATOR_OPEN = 2,
           ELEVATOR_CLOSED = 3 } state;
    struct psg* waiting;
    struct psg* riding;
    pthread_mutex_t lock;
    pthread_mutex_t psg_lock;
    pthread_cond_t cond;
} elevators[ELEVATORS];

// MUST hold lock and check for sufficient capacity before calling this,
// and MUST NOT be in the riding queue. adds to end of queue.
void add_to_list(struct psg** head, struct psg* passenger) {
    struct psg* p = *head;
    if (p == NULL)
        *head = passenger;
    else {
        while (p->next != NULL) {
            p = p->next;
        }
        p->next = passenger;
    }
}

// Must hold lock, does not update occupancy.
void remove_from_list(struct psg** head, struct psg* passenger) {
    struct psg* p = *head;
    struct psg* prev = NULL;
    while (p != passenger) {
        prev = p;
        p = (p)->next;
        // don't crash because you got asked to remove someone that isn't on this
        // list
        assert(p != NULL);
    }
    // removing the head - must fix head pointer too
    if (prev == NULL)
        *head = p->next;
    else
        prev->next = p->next;
    passenger->next = NULL;
}

void scheduler_init() {	
    for (int i = 0; i < PASSENGERS; i++) {
        passengers[i].id = i;
        passengers[i].from_floor = -1;
        passengers[i].to_floor = -1;
        passengers[i].state = WAITING;
        passengers[i].elenumber = -1;
        passengers[i].next = NULL;
        pthread_cond_init(&passengers[i].cond, NULL);
    }

    for (int i = 0; i < ELEVATORS; i++) {
        elevators[i].current_floor=0;		
        elevators[i].direction=-1;
        elevators[i].occupancy=0;
        elevators[i].requests = 0;
        elevators[i].state=ELEVATOR_CLOSED;
        elevators[i].riding = NULL;
        elevators[i].waiting = NULL;
        pthread_mutex_init(&elevators[i].lock, NULL);
        pthread_mutex_init(&elevators[i].psg_lock, NULL);
        pthread_cond_init(&elevators[i].cond, NULL);
    }
    pthread_mutex_init(&request_lock, NULL);
}

void passenger_request(int passenger, int from_floor,
                       int to_floor,void (*enter)(int, int),
                       void(*exit)(int, int))
{
    passengers[passenger].from_floor = from_floor;
    passengers[passenger].to_floor = to_floor;

    int min_ele = 0;
    pthread_mutex_lock(&request_lock);
    for (int i = 1; i < ELEVATORS; i++) {
        if (elevators[i].requests < elevators[min_ele].requests)
            min_ele = i;
    }
    elevators[min_ele].requests++;
    passengers[passenger].elenumber = min_ele;
    pthread_mutex_unlock(&request_lock);

    pthread_mutex_lock(&elevators[passengers[passenger].elenumber].psg_lock);
    elevators[passengers[passenger].elenumber].riding = &passengers[passenger];

    pthread_mutex_lock(&elevators[passengers[passenger].elenumber].lock);
    int waiting = 1;
    while(waiting)
    {
        pthread_cond_wait(&passengers[passenger].cond, &elevators[passengers[passenger].elenumber].lock);
        if(elevators[passengers[passenger].elenumber].current_floor == from_floor && elevators[passengers[passenger].elenumber].state == ELEVATOR_OPEN && elevators[passengers[passenger].elenumber].occupancy==0) {
            enter(passenger, passengers[passenger].elenumber);
            elevators[passengers[passenger].elenumber].occupancy++;
            waiting=0;
            pthread_cond_signal(&elevators[passengers[passenger].elenumber].cond);
        }
    }

    int riding=1;
    while(riding)
    {
        pthread_cond_wait(&passengers[passenger].cond, &elevators[passengers[passenger].elenumber].lock);
        if(elevators[passengers[passenger].elenumber].current_floor == to_floor && elevators[passengers[passenger].elenumber].state == ELEVATOR_OPEN) {
            exit(passenger, passengers[passenger].elenumber);
            elevators[passengers[passenger].elenumber].occupancy--;
            elevators[passengers[passenger].elenumber].requests--;
            riding=0;
            pthread_cond_signal(&elevators[passengers[passenger].elenumber].cond);
        }
    }
    pthread_mutex_unlock(&elevators[passengers[passenger].elenumber].lock);
    pthread_mutex_unlock(&elevators[passengers[passenger].elenumber].psg_lock);
}

void elevator_ready(int elevator, int at_floor,
                    void(*move_direction)(int, int),
                    void(*door_open)(int),
                     void(*door_close)(int)) 
{
    pthread_mutex_lock(&elevators[elevator].lock);
    while (elevators[elevator].riding == NULL)
    {
        pthread_cond_wait(&elevators[elevator].cond, &elevators[elevator].lock);
    }

    if (elevators[elevator].state == ELEVATOR_ARRIVED) {
        elevators[elevator].state = ELEVATOR_OPEN;
        door_open(elevator);
        pthread_cond_signal(&elevators[elevator].riding->cond);
        pthread_mutex_unlock(&elevators[elevator].lock);
        usleep(5);
    }
    else if (elevators[elevator].state == ELEVATOR_OPEN) {
        elevators[elevator].state = ELEVATOR_CLOSED;
        door_close(elevator);
        pthread_mutex_unlock(&elevators[elevator].lock);
    }
    else {
        int dest;
        if (elevators[elevator].occupancy) {
            dest = elevators[elevator].riding->to_floor;
        } else {
            dest = elevators[elevator].riding->from_floor;
        }
        elevators[elevator].direction = (dest > at_floor)? 1:-1;
        if (at_floor==0)
            elevators[elevator].direction=1;
        else if (at_floor==FLOORS-1)
            elevators[elevator].direction=-1;
        move_direction(elevator,elevators[elevator].direction);
        elevators[elevator].current_floor=at_floor+elevators[elevator].direction;
        if (elevators[elevator].current_floor == dest)
            elevators[elevator].state = ELEVATOR_ARRIVED;
        pthread_mutex_unlock(&elevators[elevator].lock);
    }
}
