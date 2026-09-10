# MLFQ Scheduler Simulator

Simulador discreto, escrito en C, de un planificador **Multi-Level Feedback
Queue (MLFQ)**. Lee procesos desde un CSV, ejecuta la simulación ciclo a ciclo
y escribe las métricas finales en otro CSV.

## Compilación y uso

```bash
make                 # compila ./mlfq
make test            # ejecuta las pruebas
./mlfq               # usa data/processes.csv, boost 20 y results.csv
./mlfq entrada.csv 10 salida.csv
./mlfq entrada.csv 10 salida.csv trazabilidad.log
```

Los argumentos opcionales, en orden, son: archivo de entrada, intervalo de
boost, archivo de salida y archivo de trazabilidad. El intervalo debe ser un
entero mayor que cero. Si no se especifica el cuarto argumento, el log se
guarda como `simulation.log`.

## Entrada

El archivo debe incluir una cabecera (la primera línea se omite) y usar este
formato:

```csv
PID,arrival_time,burst_time
P1,0,8
P2,1,4
P3,2,9
P4,3,5
```

Se admiten como máximo 100 procesos. Las líneas malformadas, con `arrival_time`
negativo o `burst_time <= 0`, se rechazan con un mensaje en stderr. Si no queda
ningún proceso válido, el programa termina con error.

## Política MLFQ implementada

| Cola | Prioridad | Quantum |
|---|---:|---:|
| Q0 | Alta | 2 ciclos |
| Q1 | Media | 4 ciclos |
| Q2 | Baja | 8 ciclos |

- Cada llegada entra en Q0.
- Se despacha siempre la cola lista de mayor prioridad; dentro de una cola se
  respeta FIFO (Round Robin).
- Si un proceso agota su quantum sin terminar, baja una cola; en Q2 se mantiene
  en Q2.
- Una llegada a una cola de mayor prioridad interrumpe al proceso activo al
  inicio del siguiente ciclo. El proceso interrumpido conserva el quantum que
  le faltaba consumir.
- Cada `boost_interval` ciclos, todos los procesos listos **y el proceso que
  estaba ejecutándose** vuelven a Q0; su quantum se reinicia a 2 ciclos.
- Un proceso que termina antes de agotar el quantum no se demueve.

## Salida y métricas

La salida contiene las siguientes columnas:

```csv
PID,Arrival,Burst,Start,Finish,Response,Turnaround,Waiting
```

- `Response = Start - Arrival`
- `Turnaround = Finish - Arrival`
- `Waiting = Turnaround - Burst`

Con el archivo `data/processes.csv` y boost de 20, la simulación dura 26 ciclos
y produce:

| PID | Arrival | Burst | Start | Finish | Response | Turnaround | Waiting |
|---|---:|---:|---:|---:|---:|---:|---:|
| P1 | 0 | 8 | 0 | 23 | 0 | 23 | 15 |
| P2 | 1 | 4 | 2 | 14 | 1 | 13 | 9 |
| P3 | 2 | 9 | 4 | 26 | 2 | 24 | 15 |
| P4 | 3 | 5 | 6 | 21 | 3 | 18 | 13 |

## Trazabilidad

Cada simulación genera un log de texto. Cada línea identifica el ciclo y el
tipo de evento, por ejemplo:

```text
[cycle=20] BOOST_START      all active and ready processes move to Q0
[cycle=20] BOOST_MOVE       process=P1 Q2->Q0 quantum=2
[cycle=20] DISPATCH         process=P4 queue=Q0 quantum_left=2
[cycle=20] EXECUTE          process=P4 queue=Q0 remaining=1 quantum_left=2
```

El archivo registra el inicio y fin de la simulación, llegadas, procesos listos,
despachos, primera ejecución, cada ciclo ejecutado, inactividad, preempción,
democión, boost y finalización. Esto permite reconstruir cómo se obtuvieron las
métricas de `results.csv`.

## Estructura

```text
src/process.*    entidad y estado del proceso
src/queue.*      cola FIFO circular
src/scheduler.*  reglas MLFQ
src/metrics.*    cálculo de métricas
src/io.*         lectura y escritura CSV
src/logger.*     trazabilidad de la simulación
src/main.c       bucle de simulación y línea de comandos
tests/           pruebas unitarias
```

Las decisiones y limitaciones de diseño se documentan en [DESIGN.md](DESIGN.md).
