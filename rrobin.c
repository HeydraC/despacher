#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<signal.h>
#include<sys/mman.h>
#include<semaphore.h>
#include<fcntl.h>
#include "queue.h"

pid_t child;
struct process *p; //Información del proceso en el procesador
int *seconds; //Segundos que han pasado desde que inició el programa
queue procQueue;

//Proceso que tiene el procesador
void process(int procTime, pid_t pid){
	sem_t* process = sem_open("process", O_CREAT, 0600, 0);


	for (int i = 0; i < procTime; ++i){
		++*seconds;
		sem_wait(process);//Espera a sincronizar con el segundo
		printf("Segundo %d: #%d EXECUTING\n", *seconds, pid);
	}

	++*seconds;
	p->procTime = -1;//Indica que el proceso ha muerto

	sem_close(process);
	exit(0);
}

//Encargado de llevar cuenta de los segundos
void time(){
	sem_t* process = sem_open("process", O_CREAT, 0600, 0);
	sem_t* ready = sem_open("ready", O_CREAT, 0600, 0);
	sem_t* dispatch = sem_open("dispatch", O_CREAT, 0600, 0);
	
	sem_wait(ready); //Espera a que haya un proceso listo
	
	while (procQueue.size > 0){ //Mientras haya procesos que ejecutar
		sleep(1);

		--p->quantum;

		//Si se termina el quantum con procesos esperando o termina el proceso
		if ((p->quantum <= 0 && procQueue.size > 1) || p->procTime < 0){

			if (p->procTime < 0) --procQueue.size; //Si muere un proceso ya no lo cuento en la cola

			sem_post(dispatch);
			sem_wait(ready); //Avisa y espera que le avisen de vuelta
		}else{
			sem_post(process);
		}

	}

	sem_close(process);
	sem_close(dispatch);
	sem_close(ready);
	sem_unlink("process"); //El padre elmina los semáforos con nombre
	sem_unlink("dispatch");
	sem_unlink("ready");
}

void dispatcher(){
	sem_t* dispatch = sem_open("dispatch", O_CREAT, 0600, 0);
	sem_t* ready = sem_open("ready", O_CREAT, 0600, 0);
	int start;

	printf("Segundo 0: ");

	while(pop(&procQueue, p)){ //Mientras haya procesos en la cola
		printf("#%d BEGIN\n", p->pid);
		start = *seconds;
	
		child = fork();

		if (!child) process(p->procTime - 1, p->pid); //El proceso hijo será el que ocupe el procesador
		//El primer segundo del proceso es el del BEGIN
		
		sem_post(ready);
		sem_wait(dispatch);//Espera a que termine el quantum

		if (p->procTime > 0){//Si le queda tiempo de ejecución se guarda otra vez
			kill(child, SIGKILL);//Mata al proceso que estaba activo

			p->procTime -= *seconds - start;
			
			printf("Segundo %d: #%d SUSPENSION ", *seconds, p->pid);
			
			push(&procQueue, p->pid, p->procTime);
			sem_post(ready);
		}else{
			printf("Segundo %d: #%d END ", *seconds, p->pid);
		}
	}

	puts("");
	sem_post(ready);

	sem_close(dispatch);
	sem_close(ready);
}

int main(){

	//Se crea una sección de memoria compartida y se asigna
	p = (struct process*)mmap(
		        NULL, sizeof(int) + sizeof(struct process), PROT_READ | PROT_WRITE,
		        MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	seconds = (int*) (p + 1);
	//(Espacio de p)|(Espacio de seconds)|(Final del heap)

	*seconds = 0;

	QueueInit(&procQueue, 4); //Se inicializa una cola con un quantum de 4 segundos

	push(&procQueue, 0, 8); //Proceso de 8 segundos
	push(&procQueue, 1, 16); //Proceso de 16 segundos
	push(&procQueue, 2, 3); //Proceso de 3 segundos
	push(&procQueue, 3, 1); //Proceso de 1 segundo
	

	child = fork();

	//El proceso hijo será el dispatcher y el padre llevará cuenta del tiempo
	if (!child){
		dispatcher();
	}else{
		time();
	}
	
	QueueDestroy(&procQueue); //Se libera la memoria asignada a la cola

	return 0;
}
