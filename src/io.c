#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "io.h"
#include "metrics.h"

#define MAX_LINE 256
#define MAX_INPUT_PROCESSES 100

static int is_blank(const char* s) {
    while (*s) {
        if (*s != ' ' && *s != '\t' && *s != '\r' && *s != '\n') return 0;
        s++;
    }
    return 1;
}

/* Valida el payload e imprime un mensaje claro antes de rechazar la línea */
static int validate_line(const char* name, int arrival, int burst) {
    if (burst <= 0) {
        fprintf(stderr, "Error de datos: burst_time (%d) debe ser > 0 en proceso '%s'\n",
                burst, name);
        return 0;
    }
    if (arrival < 0) {
        fprintf(stderr, "Error de datos: arrival_time (%d) no puede ser negativo en proceso '%s'\n",
                arrival, name);
        return 0;
    }
    return 1;
}

Process** io_read_processes(const char* filename, int* count) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Error: no se pudo abrir el archivo '%s'\n", filename);
        return NULL;
    }

    Process** processes = malloc(sizeof(Process*) * MAX_INPUT_PROCESSES);
    if (!processes) {
        fprintf(stderr, "Error: memoria insuficiente al leer procesos\n");
        fclose(f);
        return NULL;
    }

    char line[MAX_LINE];
    int n = 0;
    int line_num = 0;

    while (fgets(line, sizeof(line), f)) {
        line_num++;
        if (line_num == 1 || is_blank(line)) continue; /* saltar cabecera y vacías */

        char name[MAX_NAME_LEN];
        int arrival, burst;
        if (sscanf(line, "%15[^,],%d,%d", name, &arrival, &burst) != 3) {
            fprintf(stderr, "Error de formato en línea %d: '%s'\n", line_num, line);
            continue;
        }

        if (!validate_line(name, arrival, burst)) continue;

        if (n >= MAX_INPUT_PROCESSES) {
            fprintf(stderr, "Error: se excedió el máximo de %d procesos\n", MAX_INPUT_PROCESSES);
            for (int i = 0; i < n; i++) process_destroy(processes[i]);
            free(processes);
            fclose(f);
            *count = 0;
            return NULL;
        }

        Process* p = process_create(name, arrival, burst);
        if (!p) {
            fprintf(stderr, "Error: memoria insuficiente al crear proceso '%s'\n", name);
            for (int i = 0; i < n; i++) process_destroy(processes[i]);
            free(processes);
            fclose(f);
            *count = 0;
            return NULL;
        }
        processes[n++] = p;
    }

    fclose(f);
    *count = n;
    return processes;
}

int io_write_results(const char* filename, Process** processes, int count) {
    FILE* f = fopen(filename, "w");
    if (!f) {
        fprintf(stderr, "Error: no se pudo crear el archivo de resultados '%s'\n", filename);
        return 0;
    }

    fprintf(f, "PID,Arrival,Burst,Start,Finish,Response,Turnaround,Waiting\n");
    for (int i = 0; i < count; i++) {
        Process* p = processes[i];
        Metrics m = metrics_calculate(p);
        fprintf(f, "%s,%d,%d,%d,%d,%d,%d,%d\n",
                p->name, p->arrival_time, p->burst_time,
                p->start_time, p->finish_time,
                m.response_time, m.turnaround_time, m.waiting_time);
    }

    fclose(f);
    return 1;
}
