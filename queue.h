#ifndef QUEUE_H
#define QUEUE_H

#include<stdio.h>
#include<stdlib.h>

typedef struct{
	void *prev;
	void *next;
	pid_t pid;
	int procTime;
}node;

typedef struct{
	node *first;
	node *last;
	int quantum;
	int size;
}queue;

struct process{
	pid_t pid;
	int procTime;
	int quantum;
};

void QueueInit(queue *q, int quantum);

void QueueDestroy(queue *q);

void push(queue *q, pid_t pid, int time);

pid_t pop(queue *q, struct process* p);

#endif
