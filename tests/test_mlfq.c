#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "logger.h"
#include "process.h"
#include "queue.h"
#include "scheduler.h"
#include "metrics.h"

static int tests_run = 0;
static int tests_failed = 0;

#define CHECK(cond, msg) do { \
    tests_run++; \
    if (!(cond)) { \
        printf("FALLO: %s\n", msg); \
        tests_failed++; \
    } \
} while (0)

void test_process_creation(void) {
    Process* p = process_create("P1", 0, 8);
    CHECK(p != NULL, "crear proceso no devuelve NULL");
    if (p) {
        CHECK(p->arrival_time == 0, "arrival_time inicial");
        CHECK(p->burst_time == 8, "burst_time inicial");
        CHECK(p->remaining_time == 8, "remaining_time = burst_time");
        CHECK(p->state == STATE_NEW, "estado inicial es NEW");
        CHECK(p->current_queue == 0, "cola inicial es 0");
        process_destroy(p);
    }
}

void test_queue_operations(void) {
    Queue q;
    queue_init(&q, 2);
    CHECK(queue_is_empty(&q), "cola inicia vacía");
    CHECK(q.quantum == 2, "quantum configurado");

    Process* p1 = process_create("P1", 0, 5);
    Process* p2 = process_create("P2", 0, 3);
    queue_enqueue(&q, p1);
    queue_enqueue(&q, p2);
    CHECK(queue_size(&q) == 2, "cola agrega 2 procesos");

    Process* first = queue_dequeue(&q);
    CHECK(first == p1, "FIFO: primero en salir es p1");
    CHECK(queue_size(&q) == 1, "cola decrece tras dequeue");

    queue_dequeue(&q);
    CHECK(queue_is_empty(&q), "cola vacía tras drenar");
}

void test_demotion_logic(void) {
    Scheduler s;
    scheduler_init(&s, 20);
    Process* p = process_create("P1", 0, 10);
    scheduler_add_process(&s, p); /* entra a Q0 */

    CHECK(p->current_queue == 0, "proceso comienza en Q0");
    CHECK(scheduler_get_next(&s) == p, "se retira de Q0 antes de demover");
    scheduler_demote(&s, p);
    CHECK(p->current_queue == 1, "Q0 -> Q1 tras democión");
    CHECK(scheduler_get_next(&s) == p, "se retira de Q1 antes de demover");
    scheduler_demote(&s, p);
    CHECK(p->current_queue == 2, "Q1 -> Q2 tras democión");
    CHECK(scheduler_get_next(&s) == p, "se retira de Q2 antes de demover");
    scheduler_demote(&s, p);
    CHECK(p->current_queue == 2, "Q2 permanece (no baja más)");
    process_destroy(p);
}

void test_boost_logic(void) {
    Scheduler s;
    scheduler_init(&s, 20);
    Process* p = process_create("P1", 0, 10);
    scheduler_add_process(&s, p);
    scheduler_get_next(&s);
    scheduler_demote(&s, p); /* ahora en Q1 */
    scheduler_get_next(&s);
    scheduler_demote(&s, p); /* ahora en Q2 */

    scheduler_boost(&s);
    Process* selected = scheduler_get_next(&s);
    CHECK(selected == p, "boost devuelve el proceso a Q0 y Q0 lo selecciona");
    CHECK(p->current_queue == 0, "cola reiniciada a 0 tras boost");
    process_destroy(p);
}

void test_metrics_calculation(void) {
    Process* p = process_create("P2", 2, 6);
    p->first_response_time = 4;
    p->finish_time = 20;
    p->state = STATE_TERMINATED;

    CHECK(metrics_can_calculate(p), "métricas calculables en estado TERMINATED");
    Metrics m = metrics_calculate(p);
    CHECK(m.response_time == 2, "response = 4 - 2");
    CHECK(m.turnaround_time == 18, "turnaround = 20 - 2");
    CHECK(m.waiting_time == 12, "waiting = 18 - 6");
    process_destroy(p);
}

void test_priority_selection(void) {
    Scheduler s;
    scheduler_init(&s, 20);
    Process* low = process_create("LOW", 0, 10);
    Process* high = process_create("HIGH", 0, 2);

    scheduler_add_process(&s, low);
    scheduler_get_next(&s);
    scheduler_demote(&s, low);
    scheduler_get_next(&s);
    scheduler_demote(&s, low); /* LOW en Q2 */
    scheduler_add_process(&s, high); /* HIGH en Q0 */

    Process* selected = scheduler_get_next(&s);
    CHECK(selected == high, "se elige la cola de mayor prioridad (Q0)");
    process_destroy(low);
    process_destroy(high);
}

void test_preemption_and_boost(void) {
    Scheduler s;
    scheduler_init(&s, 20);
    Process* low = process_create("LOW", 0, 10);
    Process* high = process_create("HIGH", 1, 1);

    scheduler_add_process(&s, low);
    CHECK(scheduler_get_next(&s) == low, "LOW se selecciona inicialmente");
    scheduler_demote(&s, low); /* LOW pasa a Q1 tras agotar Q0 */
    Process* running = scheduler_get_next(&s);
    running->quantum_remaining = 2; /* ya consumió parte de su quantum de Q1 */
    scheduler_add_process(&s, high);
    CHECK(scheduler_has_higher_priority_ready(&s, running->current_queue),
          "Q0 listo debe preemptar a Q1");
    scheduler_requeue(&s, running);
    CHECK(scheduler_get_next(&s) == high, "se elige Q0 tras la preempción");
    CHECK(low->quantum_remaining == 2, "la preempción conserva quantum pendiente");
    low->quantum_remaining = 1;

    scheduler_boost(&s);
    CHECK(low->current_queue == 0, "boost mueve proceso pendiente a Q0");
    CHECK(low->quantum_remaining == 2, "boost reinicia quantum de Q0");
    CHECK(scheduler_get_next(&s) == low, "LOW queda listo para reanudar en Q0");
    process_destroy(low);
    process_destroy(high);
}

void test_logger(void) {
    const char* filename = "/tmp/mlfq_logger_test.log";
    Logger logger;
    char line[128];
    FILE* f;

    CHECK(logger_init(&logger, filename), "se crea el archivo de log");
    logger_log(&logger, 7, "TEST_EVENT", "process=P1 value=%d", 42);
    CHECK(logger_close(&logger), "se cierra el archivo de log");

    f = fopen(filename, "r");
    CHECK(f != NULL, "se puede leer el archivo de log");
    if (f) {
        fgets(line, sizeof(line), f);
        fgets(line, sizeof(line), f);
        CHECK(strstr(line, "[cycle=7]") != NULL, "el log contiene el ciclo");
        CHECK(strstr(line, "TEST_EVENT") != NULL, "el log contiene el evento");
        fclose(f);
    }
    remove(filename);
}

int main(void) {
    test_process_creation();
    test_queue_operations();
    test_demotion_logic();
    test_boost_logic();
    test_metrics_calculation();
    test_priority_selection();
    test_preemption_and_boost();
    test_logger();

    printf("Pruebas ejecutadas: %d, fallos: %d\n", tests_run, tests_failed);
    return tests_failed == 0 ? 0 : 1;
}
