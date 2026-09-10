#include <stdlib.h>
#include <string.h>
#include "process.h"

Process* process_create(const char* name, int arrival, int burst) {
    Process* p = malloc(sizeof(Process));
    if (!p) return NULL;

    strncpy(p->name, name, sizeof(p->name) - 1);
    p->name[sizeof(p->name) - 1] = '\0'; /* asegurar terminación */

    static int next_pid = 1;
    p->pid = next_pid++;
    p->arrival_time = arrival;
    p->burst_time = burst;
    p->remaining_time = burst;
    p->start_time = -1;
    p->finish_time = -1;
    p->first_response_time = -1;
    p->current_queue = 0;
    p->quantum_remaining = 0;
    p->state = STATE_NEW;
    return p;
}

void process_destroy(Process* p) {
    free(p);
}

void process_reset(Process* p) {
    p->remaining_time = p->burst_time;
    p->start_time = -1;
    p->finish_time = -1;
    p->first_response_time = -1;
    p->current_queue = 0;
    p->quantum_remaining = 0;
    p->state = STATE_NEW;
}
