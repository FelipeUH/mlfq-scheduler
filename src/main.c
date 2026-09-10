#include <stdio.h>
#include <stdlib.h>
#include "scheduler.h"
#include "io.h"
#include "logger.h"
#include "metrics.h"

#define DEFAULT_INPUT  "data/processes.csv"
#define DEFAULT_OUTPUT "results.csv"
#define DEFAULT_BOOST  20
#define DEFAULT_LOG     "simulation.log"

/* Devuelve 1 cuando todos los procesos están terminados */
static int all_terminated(Process** procs, int count) {
    for (int i = 0; i < count; i++) {
        if (procs[i]->state != STATE_TERMINATED) return 0;
    }
    return 1;
}

static void destroy_processes(Process** processes, int count) {
    if (!processes) return;
    for (int i = 0; i < count; i++) process_destroy(processes[i]);
    free(processes);
}

int main(int argc, char* argv[]) {
    const char* input = DEFAULT_INPUT;
    const char* output = DEFAULT_OUTPUT;
    const char* log_file = DEFAULT_LOG;
    int boost = DEFAULT_BOOST;

    if (argc > 1) input = argv[1];
    if (argc > 2) boost = atoi(argv[2]);
    if (argc > 3) output = argv[3];
    if (argc > 4) log_file = argv[4];

    if (boost <= 0) {
        fprintf(stderr, "Error: el intervalo de boost debe ser > 0\n");
        return 1;
    }

    int count = 0;
    Process** processes = io_read_processes(input, &count);
    if (!processes || count == 0) {
        fprintf(stderr, "Error: no se encontraron procesos válidos en '%s'\n", input);
        free(processes);
        return 1;
    }

    Logger logger;
    if (!logger_init(&logger, log_file)) {
        destroy_processes(processes, count);
        return 1;
    }
    logger_log(&logger, 0, "SIMULATION_START",
               "input=%s processes=%d boost_interval=%d output=%s",
               input, count, boost, output);

    Scheduler sched;
    scheduler_init(&sched, boost);
    scheduler_set_logger(&sched, &logger);

    /* Cola de llegadas ordenada por arrival_time (Factory: origen desde CSV) */
    int arrivals[MAX_PROCESSES];
    for (int i = 0; i < count; i++) arrivals[i] = -1;
    for (int i = 0; i < count; i++) {
        int pos = i;
        while (pos > 0 && processes[arrivals[pos - 1]]->arrival_time > processes[i]->arrival_time) {
            arrivals[pos] = arrivals[pos - 1];
            pos--;
        }
        arrivals[pos] = i;
    }

    int arrival_idx = 0;
    Process* running = NULL;
    int quantum_left = 0;

    while (!all_terminated(processes, count)) {
        /* Ingresar procesos que llegan en este ciclo */
        while (arrival_idx < count &&
               processes[arrivals[arrival_idx]]->arrival_time <= sched.current_cycle) {
            Process* p = processes[arrivals[arrival_idx]];
            p->state = STATE_READY;
            logger_log(&logger, sched.current_cycle, "ARRIVAL",
                       "process=%s arrival=%d burst=%d", p->name,
                       p->arrival_time, p->burst_time);
            if (!scheduler_add_process(&sched, p)) {
                logger_close(&logger);
                destroy_processes(processes, count);
                return 1;
            }
            arrival_idx++;
        }

        /* Priority boost periódico */
        if (sched.current_cycle > 0 && sched.current_cycle % sched.boost_interval == 0) {
            logger_log(&logger, sched.current_cycle, "BOOST_START", "all active and ready processes move to Q0");
            /* El proceso activo también participa en el boost. */
            if (running != NULL) {
                logger_log(&logger, sched.current_cycle, "BOOST_ACTIVE",
                           "process=%s Q%d->Q0", running->name, running->current_queue);
                if (!scheduler_add_process(&sched, running)) {
                    logger_close(&logger);
                    destroy_processes(processes, count);
                    return 1;
                }
                running = NULL;
            }
            if (!scheduler_boost(&sched)) {
                logger_close(&logger);
                destroy_processes(processes, count);
                return 1;
            }
        }

        /* Una cola superior lista preempta desde el siguiente ciclo. */
        if (running != NULL &&
            scheduler_has_higher_priority_ready(&sched, running->current_queue)) {
            logger_log(&logger, sched.current_cycle, "PREEMPT",
                       "process=%s queue=Q%d", running->name, running->current_queue);
            if (!scheduler_requeue(&sched, running)) {
                logger_close(&logger);
                destroy_processes(processes, count);
                return 1;
            }
            running = NULL;
        }

        /* Tomar el siguiente proceso si no hay uno corriendo */
        if (running == NULL) {
            running = scheduler_get_next(&sched);
            if (running != NULL) {
                quantum_left = running->quantum_remaining;
                if (running->first_response_time < 0) {
                    running->first_response_time = sched.current_cycle;
                }
                if (running->start_time < 0) {
                    running->start_time = sched.current_cycle;
                    logger_log(&logger, sched.current_cycle, "FIRST_RUN",
                               "process=%s response=%d", running->name,
                               running->first_response_time - running->arrival_time);
                }
                running->state = STATE_RUNNING;
            } else if (arrival_idx >= count) {
                /* Sin procesos corriendo ni pendientes: termina */
                break;
            }
        }

        if (running != NULL) {
            logger_log(&logger, sched.current_cycle, "EXECUTE",
                       "process=%s queue=Q%d remaining=%d quantum_left=%d",
                       running->name, running->current_queue, running->remaining_time,
                       quantum_left);
            running->remaining_time--;
            quantum_left--;
            running->quantum_remaining = quantum_left;
            sched.current_cycle++;

            if (running->remaining_time <= 0) {
                running->finish_time = sched.current_cycle;
                running->state = STATE_TERMINATED;
                logger_log(&logger, sched.current_cycle, "FINISH",
                           "process=%s turnaround=%d", running->name,
                           running->finish_time - running->arrival_time);
                running = NULL;
            } else if (quantum_left <= 0) {
                if (!scheduler_demote(&sched, running)) {
                    logger_close(&logger);
                    destroy_processes(processes, count);
                    return 1;
                }
                running = NULL;
            }
        } else {
            /* Tiempo ocioso: no hay procesos listos aún */
            logger_log(&logger, sched.current_cycle, "IDLE", "no ready processes");
            sched.current_cycle++;
        }
    }

    if (!io_write_results(output, processes, count)) {
        logger_close(&logger);
        destroy_processes(processes, count);
        return 1;
    }

    logger_log(&logger, sched.current_cycle, "SIMULATION_END",
               "results=%s processes=%d", output, count);
    if (!logger_close(&logger) || logger_has_error(&logger)) {
        fprintf(stderr, "Error: no se pudo completar el archivo de log '%s'\n", log_file);
        destroy_processes(processes, count);
        return 1;
    }

    printf("Simulación completada. Resultados guardados en '%s'\n", output);
    printf("Procesos: %d | Ciclos: %d | Intervalo boost: %d\n", count, sched.current_cycle, boost);
    printf("Trazabilidad guardada en '%s'\n", log_file);

    destroy_processes(processes, count);
    return 0;
}
