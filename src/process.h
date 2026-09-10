#ifndef PROCESS_H
#define PROCESS_H

typedef enum {
    STATE_NEW,
    STATE_READY,
    STATE_RUNNING,
    STATE_WAITING,
    STATE_TERMINATED
} ProcessState;

typedef struct {
    int pid;
    char name[16];
    int arrival_time;
    int burst_time;
    int remaining_time;
    int start_time;
    int finish_time;
    int first_response_time;
    int current_queue;
    /* Se conserva al ser interrumpido por una cola de mayor prioridad. */
    int quantum_remaining;
    ProcessState state;
} Process;

Process* process_create(const char* name, int arrival, int burst);
void process_destroy(Process* p);
void process_reset(Process* p);

#endif
