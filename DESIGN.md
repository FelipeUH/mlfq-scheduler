# Diseño del simulador MLFQ

## Organización del código

```text
main.c (aplicación: argumentos, llegadas y ciclo de simulación)
 ├── io.c        (infraestructura: CSV)
 ├── logger.c    (infraestructura: trazabilidad)
 ├── metrics.c   (reglas de métricas)
 └── scheduler.c (política MLFQ)
      ├── queue.c   (FIFO circular)
      └── process.c (entidad Process)
```

`io.c` y `logger.c` son los módulos que abren o escriben archivos. `process`,
`queue` y `scheduler` no conocen el formato CSV. La separación reduce el
acoplamiento de la entrada/salida con las reglas del planificador, aunque no
pretende ser una implementación estricta de Clean Architecture.

## Decisiones de implementación

### Colas FIFO circulares

Cada nivel usa un arreglo circular de hasta 100 punteros a `Process`. Las
operaciones de encolar y desencolar son O(1) y preservan el orden Round Robin
dentro de cada nivel. `queue_enqueue` devuelve éxito o fallo, para que el
llamador no pierda procesos ante una cola llena.

### Estado del proceso y quantum pendiente

`Process` conserva su estado (`NEW`, `READY`, `RUNNING`, `WAITING` o
`TERMINATED`), la cola actual y `quantum_remaining`. Este último campo evita
regalar un quantum nuevo a un proceso de Q1/Q2 que fue interrumpido por una
llegada de mayor prioridad. En cambio, una democión o un priority boost sí
asignan el quantum completo de la nueva cola.

### Preempción y boost

El bucle de `main.c` procesa, en cada ciclo, las llegadas, el boost y luego la
preempción. Si hay una cola lista más prioritaria que la del proceso activo,
este se reencola en su nivel con el quantum pendiente intacto y se despacha la
cola superior.

En un boost, `main.c` reencola primero el proceso activo en Q0 y
`scheduler_boost` traslada el contenido de Q1 y Q2 a Q0. Todos ellos quedan
con el quantum de Q0. Los procesos terminados no participan.

### Validación y ciclo de vida de memoria

La lectura CSV valida el formato, `arrival_time >= 0`, `burst_time > 0`, el
límite de 100 procesos y fallos de asignación. `main.c` comprueba el intervalo
de boost y el resultado de la exportación. Al terminar —también en los errores
posteriores a la lectura— libera todos los procesos creados.

### Logging de eventos

`Logger` escribe un archivo de texto por ejecución. El `Scheduler` recibe un
puntero opcional a este componente mediante `scheduler_set_logger`; por ello,
las pruebas unitarias pueden ejecutarse sin crear un log. Los eventos del núcleo
(`READY`, `DISPATCH`, `DEMOTE`, `PREEMPT_REQUEUE` y `BOOST_MOVE`) se emiten desde
el scheduler; `main.c` registra los eventos del ciclo (`ARRIVAL`, `EXECUTE`,
`FINISH`, `IDLE`, `BOOST_START` y límites de la simulación). Cada evento incluye
el ciclo de reloj para reconstruir el orden exacto.

## Principios y patrones aplicados

| Elemento | Evidencia |
|---|---|
| SRP | `process`, `queue`, `scheduler`, `metrics`, `io`, `logger` y `main` tienen responsabilidades separadas. |
| State (ligero) | El enum `ProcessState` hace explícito el ciclo de vida de cada proceso. |
| Factory (ligero) | `process_create` centraliza la inicialización; `io_read_processes` crea entidades desde CSV. |
| DRY | La reubicación se concentra en `scheduler_add_process`, `scheduler_requeue`, `scheduler_demote` y `scheduler_boost`. |

La selección de prioridad está concentrada en `scheduler_get_next`, por lo que
es el punto natural para cambiar la política. Sin embargo, el proyecto no
implementa una interfaz de políticas intercambiables: `Scheduler` contiene tres
`Queue` concretas y `Process`/`Queue` exponen sus campos en los encabezados.
Por ello, OCP, DIP y encapsulamiento se cumplen solo de forma parcial; añadir
otra política o esconder completamente el estado requeriría una refactorización
adicional.

## Comportamiento esperado

- Un boost muy frecuente mantiene los procesos cerca de Q0 y reduce el tiempo
  que pasan en niveles bajos, pero aumenta los cambios de contexto.
- Sin boost, una carga sostenida de procesos nuevos en Q0 puede postergar
  indefinidamente procesos de Q2 (starvation).
- Un quantum pequeño en Q0 favorece la respuesta de tareas cortas, a cambio de
  más cambios de contexto.

## Verificación actual

`make` compila con `-Wall -Wextra -std=c99`. `make test` ejecuta 38 aserciones
que cubren creación de procesos, FIFO, democión, boost, prioridad, preempción,
conservación del quantum y métricas. La ejecución principal genera además una
traza verificable en `simulation.log`.
