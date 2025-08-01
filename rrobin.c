#include<pthread.h>
#include<stdio.h>
#include<unistd.h>
#include<semaphore.h>
#include<string.h>
#include "queue.h"

struct process p;
int seconds, end;
short bit;
sem_t process, ready, dispatch;
SuperCola jobList;

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

		if ((p.quantum <= 0 && jobList.size > 0) || p.procTime < 0){
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

	while (SuperColaPop(&jobList, &p)){
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

			if (p.priority < 3) ++p.priority;
			
			SuperColaPush(&jobList, p);
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

struct process filtrarInput(char* linea, pid_t pidCount){
	struct process  temp;
	int arr, prio, cpu, p, s, m, d;
	sscanf(linea, "%d, %d, %d, %d, %d, %d, %d", &arr, &prio, &cpu, &p, &s, &m, &d);
		temp.pid=pidCount;
		temp.arrival=arr;
		temp.priority=prio;
		temp.procTime=cpu;
		temp.printer=p;
		temp.scanner=s;
		temp.modem=m;
		temp.dvd=d;
		temp.quantum=0;
	
	return temp;
	
}

int main(int argc, char *argv[]){
	pthread_t t[2];
	struct process temp;
	char *nombreArchivo= argv[1];

	sem_init(&process, 0, 0);
	sem_init(&ready, 0, 0);
	sem_init(&dispatch, 0, 0);

	bit = 1;

	//El primer segundo es de BEGIN
	seconds = 1;
	//Placeholder antes de tener el primer proceso
	end = 999; 
	
	SuperColaInit(&jobList); //Se inicializa la cola retroalimentada

	FILE *i=fopen(nombreArchivo, "r");
	if (!i)
		return EXIT_FAILURE;
	char buffer[256];
	pid_t pidCount=0;
	while (fgets(buffer, sizeof(buffer), i)){
	if(strlen(buffer)>1){
		temp=filtrarInput(buffer, pidCount++);
		SuperColaPush(&jobList, temp);
		}
	}
	fclose(i);

// 	procInit(&aux, 0, 8, 1);
// 	SuperColaPush(&jobList, aux); //Proceso de 8 segundos
// 
// 	procInit(&aux, 1, 16, 1);
// 	SuperColaPush(&jobList, aux); //Proceso de 16 segundos
// 
// 	procInit(&aux, 2, 3, 1);
// 	SuperColaPush(&jobList, aux); //Proceso de 3 segundos
// 
// 	procInit(&aux, 3, 1, 1);
// 	SuperColaPush(&jobList, aux); //Proceso de 1 segundo

	pthread_create(t, NULL, dispatcher, NULL);
	pthread_create(t + 1, NULL, processor, NULL);

	timer();

	pthread_cancel(t[1]);
	pthread_join(t[0], NULL);

	SuperColaDestroy(&jobList);
	sem_destroy(&process);
	sem_destroy(&ready);
	sem_destroy(&dispatch);

	return 0;
}
