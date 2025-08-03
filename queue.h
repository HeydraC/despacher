#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>
#include <stdlib.h>

struct process
{
    pid_t pid;
    int procTime;
    int quantum;
    int priority;
    int arrival;
    int printer;
    int scanner;
    int modem;
    int dvd;
};

typedef struct node
{
    struct process p;
    struct node *next;
    struct node *prev;
} node;

// Definición de la estructura de la cola
typedef struct queue
{
    node *first;
    node *last;
    int size;
    int quantum;
} queue;

// Definición de la SuperCola (colección de colas de prioridad)
typedef struct superCola
{
    queue queues[4];
    int size;
} SuperCola;

// --- Prototipos de funciones para Queue ---
void QueueInit(queue *q, int quantum_val); // Ahora toma el quantum que la cola asigna
void QueueDestroy(queue *q);
// push ahora acepta un struct process completo
void push(queue *q, struct process new_process);
// pop ahora retorna 1 (éxito) o 0 (vacío) y rellena `p_out`
int pop(queue *q, struct process *p_out);

// pop para sacar un nodo especifico de la lista
int popPid(queue *q, struct node *p_pop);
void SuperColaInit(SuperCola *sc);

void SuperColaDestroy(SuperCola *sc);

void SuperColaPush(SuperCola *sc, struct process process_to_add);
int SuperColaPop(SuperCola *sc, struct process *p_out);

void procInit(struct process *p, int pid, int procTime, int priority);

#endif
