#include "queue.h"
#include <stdlib.h>
void QueueInit(queue *q, int quantum_val)
{
    q->first = NULL;
    q->last = NULL;
    q->size = 0;
    q->quantum = quantum_val;
} //Inicialización de los valores de la cola

void QueueDestroy(queue *q)
{
    node *current = q->first;
    while (current != NULL)
    {
        node *next_node = current->next;
        free(current);
        current = next_node;
    }
    q->first = NULL;
    q->last = NULL;
    q->size = 0;
} //Liberación de la memoria asignada a la cola

// Inserta un nuevo proceso al principio de la cola (FCFS para elementos de igual prioridad)
// Acepta un struct process completo para almacenar
void push(queue *q, struct process p)
{
    node *new_node = (node *)malloc(sizeof(node));

    if (new_node == NULL)
    {
        perror("Error asignando memoria para un nuevo nodo");
        exit(EXIT_FAILURE);
    }

    new_node->p = p; // Copia la estructura del proceso completa
    new_node->next = NULL;
    new_node->prev = NULL;

    // Si la cola está vacía
    if (q->first == NULL)
    {
        q->first = new_node;
        q->last = new_node;
    }
    else
    { // Si la cola no está vacía, añadir al principio (FCFS de los que ya están)
        new_node->next = q->first;
        q->first->prev = new_node;
        q->first = new_node;
    }
    q->size++;
}

// Saca un proceso del final de la cola (FCFS)
// Retorna 0 (false) si la cola está vacía, 1 (true) si se pudo sacar un proceso
int pop(queue *q, struct process *p_out)
{
    if (q->last == NULL)
    {
        return 0; // Cola vacía
    }

    node *node_to_pop = q->last;
    *p_out = node_to_pop->p; // Copia el proceso a la variable de salida

    q->last = node_to_pop->prev;
    if (q->last != NULL)
    {
        q->last->next = NULL;
    }
    else
    { // Si la cola queda vacía
        q->first = NULL;
    }
    free(node_to_pop);
    q->size--;

    return 1; // Proceso sacado con éxito
}

int popPid(queue *q, struct node *p_pop) //Pop de un proceso especificado
{

    if (p_pop->next == NULL && p_pop->prev == NULL)
    {
        q->first = NULL;
        q->last = NULL;
        q->size--;
        return 1;
    }
    // Caso en que este sacando el primero de la cola
    if (p_pop->prev == NULL)
    {
        p_pop->next->prev = NULL;
        q->first = p_pop->next;
        q->size--;
        return 1; // lo saco con exito
    }

    // Caso en que este sacando el ultimo de la cola
    if (p_pop->next == NULL)
    {
        p_pop->prev->next = NULL;
        q->last = p_pop->prev;
        q->size--;
        return 1;
    }

    // Caso que este sacando alguno de los intermedios
    p_pop->prev->next = p_pop->next;
    p_pop->next->prev = p_pop->prev;
    q->size--;
    return 1;
}

void SuperColaInit(SuperCola *sc)
{
    QueueInit(&sc->queues[0], 0); // Cola de Tiempo Real (P0) - Quantum de cola no aplica directamente
    QueueInit(&sc->queues[1], 3); // Cola de Prioridad 1 (P1) - Quantum 3 segundos
    QueueInit(&sc->queues[2], 2); // Cola de Prioridad 2 (P2) - Quantum 2 segundos
    QueueInit(&sc->queues[3], 1); // Cola de Prioridad 3 (P3) - Quantum 1 segundo

    sc->size = 0;
}

void SuperColaDestroy(SuperCola *sc)
{
    for (int i = 0; i < 4; ++i)
    {
        QueueDestroy(&sc->queues[i]);
    }
    sc->size = 0;
} //Liberación de la memoria de todas las colas

void SuperColaPush(SuperCola *sc, struct process process_to_add)
{

    // Asignar el quantum de la cola al campo 'quantum' del proceso para su próxima ejecución
    if (process_to_add.priority == 0)
    { // Procesos de Tiempo Real
        process_to_add.quantum = process_to_add.procTime;
    }
    else
    { // Procesos de Usuario (P1, P2, P3)
        process_to_add.quantum = sc->queues[process_to_add.priority].quantum;
    }

    ++sc->size;

    // Insertar el proceso en la cola correspondiente a su prioridad
    push(&sc->queues[process_to_add.priority], process_to_add);
}

int SuperColaPop(SuperCola *sc, struct process *p_out)
{
    // Iterar desde la prioridad más alta (0) a la más baja (3)
    for (int i = 0; i < 4; i++)
    {
        if (sc->queues[i].size > 0)
        {
            // Si la cola no está vacía, sacar el último (FCFS dentro de esa prioridad)
            --sc->size;
            return pop(&sc->queues[i], p_out);
        }
    }
    return 0; // Todas las colas están vacías
}

void procInit(struct process *p, int pid, int procTime, int priority)
{
    p->pid = pid;
    p->procTime = procTime;
    p->priority = priority;
} //Esto se suponía que era para probar, nunca lo usé y no lo quiero quitar :p
