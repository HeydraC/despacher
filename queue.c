#include "queue.h"

void QueueInit(queue *q, int quantum){
	q->first = NULL;
	q->last = NULL;
	q->quantum = quantum;
	q->size = 0;
}

void QueueDestroy(queue *q){
	node* aux;
	
	while (q->first != NULL){
		aux = q->first;
		q->first = (node*)q->first->next;
		free(aux);
	}
}

void push(queue *q, pid_t pid, int time){
	node* newNode = (node*)malloc(sizeof(node));

	if (newNode == NULL){
		perror("Error asignando memoria");
		exit(-1);
	}

	++q->size;

	newNode->prev = NULL;
	newNode->next = NULL;
	newNode->pid = pid;
	newNode->procTime = time;

	if (q->first == NULL){
		q->first = newNode;
		q->last = newNode;

		return;
	}

	newNode->next  = (void*)q->first;
	q->first->prev = (void*)newNode;
	q->first = newNode;
}

//Retorna 0(false) si no hay elementos que sacar
int pop(queue *q, struct process* p){
	if (q->last == NULL) return 0;
	
	node* aux = q->last;
	
	p->pid 	= q->last->pid;
	p->procTime = q->last->procTime;
	p->quantum = q->quantum;
	
	q->last = (node*)q->last->prev;

	if (q->last == NULL) q->first = NULL;

	free(aux);

	--q->size;

	return 1;
}
