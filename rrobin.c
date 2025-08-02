#include<pthread.h>
#include<stdio.h>
#include<unistd.h>
#include<semaphore.h>
#include<string.h>
#include "queue.h"

//Información del proceso que se está ejecutando
struct process p;
//Tiempo actual y tiempo hasta el final del proceso
int seconds, end;
//Booleano para avisar final del programa
short bit;
//Semáforos para sincronización
sem_t process, ready, dispatch;
//Cola de procesos a manejar por el dispatcher
SuperCola jobList;
//Procesos por llegar, cantidad y un iterador
struct process beforeArrival[1024];
int n, actual;
//Mayor prioridad en la cola actualmente
short priority;

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

		//Se agregan todos los procesos que llegan a un tiempo determinado
		while (actual < n && beforeArrival[actual].arrival <= seconds){
			SuperColaPush(&jobList, beforeArrival[actual]);

			if (beforeArrival[actual].priority < priority)
				priority = beforeArrival[actual].priority;
			
			++actual;
		}


		//Se acaba el quantum con procesos en espera, termina el proceso o llega un proceso de mayor prioridad
		if ((p.quantum <= 0 && jobList.size > 0) || p.procTime < 0 || priority < p.priority){
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

		//Se guarda la mayor prioridad en la cola
		priority = p.priority;

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
			printf("Segundo %d: #%d END\n", seconds, p.pid);
			++seconds;
			lastDead = 1;
			if (jobList.size > 0){
				printf("Segundo %d: ", seconds);
				sleep(1);
			}
		}
	}

	bit = 0; //Avisa el final del programa
	sem_post(&ready);
}

void sortByArrival(struct process* arr, int n){
	if (n == 1) return;

	int size1, size2, a, b, c;
	size1 = n/2;
	size2 = n - size1;

	struct process *arr1, *arr2;

	arr1 = malloc(sizeof(struct process)*size1);
	arr2 = malloc(sizeof(struct process)*size2);

	for (int i = 0; i < size1; ++i) arr1[i] = arr[i];
	for (int i = size1; i < n; ++i) arr2[i-size1] = arr[i];

	sortByArrival(arr1, size1);
	sortByArrival(arr2, size2);

	a = 0; b = 0; c = 0;

	while (a < size1 && b < size2){
		//Se ordena por tiempo de llegada y por pid
		if (arr1[a].arrival < arr2[b].arrival ||
			(arr1[a].arrival == arr2[b].arrival && arr1[a].pid < arr2[b].pid)){
			arr[c] = arr1[a];
			++a;
		}else{
			arr[c] = arr2[b];
			++b;
		}

		++c;
	}

	while (a < size1){
		arr[c] = arr1[a];
		++a; ++c;
	}

	while (b < size2){
		arr[c] = arr2[b];
		++b; ++c;
	}

	free(arr1);
	free(arr2);
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
	//Inicializando iterador y tamaño
	actual = 0;
	n = 0;
	//Placeholder antes de tener el primer proceso
	priority = -1;
	
	SuperColaInit(&jobList); //Se inicializa la cola retroalimentada

	FILE *i=fopen(nombreArchivo, "r");
	if (!i)
		return EXIT_FAILURE;
	char buffer[256];
	pid_t pidCount=0;
	while (fgets(buffer, sizeof(buffer), i)){
		if(strlen(buffer)>1){
			temp=filtrarInput(buffer, pidCount++);
			
			if (temp.arrival > 0){
				beforeArrival[n] = temp;
				++n;
			}else{
				SuperColaPush(&jobList, temp);
			}//Sólo se meten a la cola los procesos que llegan al segundo 0
		}
	}
	fclose(i);

	sortByArrival(beforeArrival, n); //Se ordenan los procesos por tiempo de llegada

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
