#include "metrics.h"

int metrics_can_calculate(Process* p) {
    return p->state == STATE_TERMINATED &&
           p->first_response_time >= 0 &&
           p->finish_time >= 0;
}

Metrics metrics_calculate(Process* p) {
    Metrics m = {0, 0, 0};
    m.response_time = p->first_response_time - p->arrival_time;
    m.turnaround_time = p->finish_time - p->arrival_time;
    m.waiting_time = m.turnaround_time - p->burst_time;
    return m;
}
