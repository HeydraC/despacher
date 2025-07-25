#include<pthread.h>
#include<stdio.h>
#include<unistd.h>
#include<semaphore.h>
#include "queue.h"

struct process p;
int seconds, end;
short bit;
sem_t process, ready, dispatch;
queue procQueue;

void* processor(){
	sem_wait(&process);

	while(1){
		for (; seconds < end; ++seconds){
			sem_wait(&process);
			printf("Segundo %d: #%d EXECUTING\n", seconds, p.pid);
			if (seconds != end - 1) sem_post(&ready);
		}

		p.procTime = -1;
		sem_post(&ready);
		sem_wait(&process);
	}
}

void timer(){
	sem_wait(&ready);

	while (bit){
		sleep(1);

		--p.quantum;

		if ((p.quantum <= 0 && procQueue.size > 0) || p.procTime < 0){
			sem_post(&dispatch);
			sem_wait(&ready);
		}else{
			sem_post(&process);
			sem_wait(&ready);
		}
	}
}

void* dispatcher(){
	int start, lastDead = 1;
	printf("Segundo 0: ");

	while (pop(&procQueue, &p)){
		printf("#%d BEGIN\n", p.pid);

		start = seconds;

		//El primer segundo del proceso es el de BEGIN
		end = seconds + p.procTime - 1;

		if (lastDead){
			sem_post(&process);
			lastDead = 0;
		}
		sem_post(&ready);
		sem_wait(&dispatch);

		if (p.procTime > 0){
			printf("Segundo %d: #%d SUSPENSION ", seconds, p.pid);
			++seconds;
			p.procTime -= seconds - start;
			
			push(&procQueue, p.pid, p.procTime);
		}else{
			printf("Segundo %d: #%d END ", seconds, p.pid);
			++seconds;
			lastDead = 1;
		}
	}

	puts("");
	bit = 0; //Avisa el final del programa
	sem_post(&ready);
}

int main(){
	pthread_t t[2];

	sem_init(&process, 0, 0);
	sem_init(&ready, 0, 0);
	sem_init(&dispatch, 0, 0);

	bit = 1;

	//El primer segundo es de BEGIN
	seconds = 1;
	//Placeholder antes de tener el primer proceso
	end = 999; 

	
	QueueInit(&procQueue, 4); //Se inicializa una cola con un quantum de 4 segundos

	push(&procQueue, 0, 8); //Proceso de 8 segundos
	push(&procQueue, 1, 16); //Proceso de 16 segundos
	push(&procQueue, 2, 3); //Proceso de 3 segundos
	push(&procQueue, 3, 1); //Proceso de 1 segundo

	pthread_create(t, NULL, dispatcher, NULL);
	pthread_create(t + 1, NULL, processor, NULL);

	timer();

	pthread_cancel(t[1]);
	pthread_join(t[0], NULL);

	QueueDestroy(&procQueue);
	sem_destroy(&process);
	sem_destroy(&ready);
	sem_destroy(&dispatch);

	return 0;
}
