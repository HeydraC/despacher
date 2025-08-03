#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include "queue.h"

// Información del proceso que se está ejecutando
struct process p;
// Tiempo actual
int seconds;
// Booleano para avisar final del programa
short bit;
// Semáforos para sincronización
sem_t ready, dispatch, blockedSem;
// Cola de procesos a manejar por el dispatcher
SuperCola jobList;
// Procesos por llegar, cantidad , un iterador y procesos que quedan por llegar
struct process beforeArrival[512];
int n, actual, nInBeforeArrival;
// Mayor prioridad en la cola actualmente
short priority;

// Vector de recursos existentes en el sistema  0: Impresoras, 1: Scanner, 2: Modem, 3: DVD
int maxInstanceResources[4] = {2, 1, 1, 2};
// Vector de recursos disponibles 0: Impresoras, 1: Scanner, 2: Modem, 3: DVD
int availableResource[4] = {2, 1, 1, 2};
// Cola de bloqueados
queue processesBlocked;

//Archivo de log
FILE* file;

//Retorna 1 si hay recursos suficientes para ejecutar el proceso, 0 de lo contrario
int canTakeResources(int needImpresoras, int needScanner, int needModem, int needDvd)
{

	if (needImpresoras > availableResource[0])
		return 0;
	if (needScanner > availableResource[1])
		return 0;
	if (needModem > availableResource[2])
		return 0;
	if (needDvd > availableResource[3])
		return 0;

	return 1;
}

//Hilo para el manejo de procesos bloqueados
void *blocked()
{
	while (bit)
	{
		sem_wait(&blockedSem);

		struct node *auxFirst = processesBlocked.last; //Carlos hizo la cola al revés
		while (auxFirst != NULL) //Se recorre la lista en busca de procesos que se les puedan dar recursos
		{
			struct process temp = auxFirst->p;
			if (canTakeResources(temp.printer, temp.scanner, temp.modem, temp.dvd))
			{
				availableResource[0] -= temp.printer;
				availableResource[1] -= temp.scanner;
				availableResource[2] -= temp.modem;
				availableResource[3] -= temp.dvd;
				//Se dan los recursos al proceso

				popPid(&processesBlocked, auxFirst); //Se saca el proceso de la cola de bloqueados
				SuperColaPush(&jobList, temp); //Se hace push del proceso a la cola retroalimentada
			}
			auxFirst = auxFirst->prev; //Cola al revés y no quiero cambiar todo lo demás
		}
		sem_post(&dispatch); //Avisa al dispatcher que terminó su tarea
	}
}

void timer()
{
	sem_wait(&ready);
	int timeSpent = 0, lastProcess;

	while (bit)
	{
		sleep(1); //Da una espera de 1 segundo entre cada impresión

		--p.quantum;
		--p.procTime; //Disminuye el tiempo restante de ejecución y del quantum
		++timeSpent;

		if (timeSpent >= 20)
			p.procTime = 0; //El proceso finaliza luego de ejecutarse por 20 sergundos seguidos

		// Se agregan todos los procesos que llegan a un tiempo determinado
		while (actual < n && beforeArrival[actual].arrival <= seconds)
		{	
			//Si hay recursos suficientes para el proceso
			if (canTakeResources(beforeArrival[actual].printer, beforeArrival[actual].scanner,
				beforeArrival[actual].modem, beforeArrival[actual].dvd) || beforeArrival[actual].priority == 0)
			{
				availableResource[0] -= beforeArrival[actual].printer;
				availableResource[1] -= beforeArrival[actual].scanner;
				availableResource[2] -= beforeArrival[actual].modem;
				availableResource[3] -= beforeArrival[actual].dvd;
				//Se le dan los recursos
				
				SuperColaPush(&jobList, beforeArrival[actual]); //Y se introduce en la cola retroalimentada

				if (beforeArrival[actual].priority < priority)
				{
					priority = beforeArrival[actual].priority;
					//Se cambia la mayor prioridad en la cola para saber que se necesita un cambio de contexto
				}
			}
			else
			{
				push(&processesBlocked, beforeArrival[actual]);
				//Si no hay recursos suficientes se mete el proceso a la cola de bloqueados
			}
			--nInBeforeArrival;
			++actual; //Se pasa al siguiente proceso por llegar
		}

		// Se acaba el quantum con procesos en espera, termina el proceso o llega un proceso de mayor prioridad
		if ((p.quantum <= 0 && jobList.size > 0) || p.procTime <= 0 || priority < p.priority)
		{
			lastProcess = p.pid;

			sem_post(&dispatch);
			sem_wait(&ready);

			if (p.pid != lastProcess) //Si el proceso sacado es el mismo, cuenta como ejecución continua del mismo
				timeSpent = 0;
		}
		else
		{
			printf("Segundo %d: #%d EXECUTING\n", seconds, p.pid);
			fprintf(file, "Segundo %d: #%d EXECUTING\n", seconds, p.pid);
			++seconds;
		}
	}
}

void *dispatcher()
{
	int firstCycle = 1;
	int f;

	//Mientras haya procesos en la cola retroalimentada o por llegar
	while ((f = SuperColaPop(&jobList, &p)) || nInBeforeArrival > 0)
	{
		if (f == 0 && nInBeforeArrival > 0)
		{
			++seconds;
			sem_post(&ready);
			sem_wait(&dispatch);
			continue;
			//Continúa la ejecución en caso de haber procesos por llegar
		}

		if (firstCycle)
		{
			printf("Segundo %d: ", seconds - 1);
			fprintf(file, "Segundo %d: ", seconds - 1);
			firstCycle = 0;
			//En caso de ser la primera vez que se imprime, ya que las demás se hacen por separado
		}

		printf("#%d BEGIN\n", p.pid); //Se indica el inicio del proceso sacado de la cola
		fprintf(file, "#%d BEGIN\n", p.pid);

		// Se guarda la mayor prioridad en la cola
		priority = p.priority;

		sem_post(&ready);
		sem_wait(&dispatch); //Se avisa a timer que terminó el cambio de contexto

		++seconds;
		//Segundo correspondiente al final del proceso actual

		if (p.procTime > 0) //Si queda tiempo de ejecución
		{
			printf("Segundo %d: #%d SUSPENSION ", seconds - 1, p.pid); //Se indica la suspensión
			fprintf(file, "Segundo %d: #%d SUSPENSION ", seconds - 1, p.pid);

			if (p.priority < 3)
				++p.priority; //Se degrada la prioridad de los procesos de usuario

			SuperColaPush(&jobList, p); //Se vuelven a meter a la cola
		}
		else
		{
			printf("Segundo %d: #%d END\n", seconds - 1, p.pid); //Se indica el final del proceso
			fprintf(file, "Segundo %d: #%d END\n", seconds - 1, p.pid);

			if (processesBlocked.size > 0)
			{

				availableResource[0] += p.printer;
				availableResource[1] += p.scanner;
				availableResource[2] += p.modem;
				availableResource[3] += p.dvd;
				sem_post(&blockedSem); //Se liberan los recursos y se revisa si se puede ejecutar algún proceso bloqueado
				sem_wait(&dispatch);
			}
			if (jobList.size > 0)
			{
				sleep(1); //Segundo entre END y BEGIN
				printf("Segundo %d: ", seconds);
				fprintf(file, "Segundo %d: ", seconds);
			}
			++seconds; //Segundo entre END y BEGIN
		}
	}

	bit = 0; // Avisa el final del programa
	sem_post(&ready);
}

//Mergesort de arreglo de procesos por llegar
void sortByArrival(struct process *arr, int n)
{
	if (n == 1)
		return;

	int size1, size2, a, b, c;
	size1 = n / 2;
	size2 = n - size1;

	struct process *arr1, *arr2;

	arr1 = malloc(sizeof(struct process) * size1);
	arr2 = malloc(sizeof(struct process) * size2);

	for (int i = 0; i < size1; ++i)
		arr1[i] = arr[i];
	for (int i = size1; i < n; ++i)
		arr2[i - size1] = arr[i];

	sortByArrival(arr1, size1);
	sortByArrival(arr2, size2);

	a = 0;
	b = 0;
	c = 0;

	while (a < size1 && b < size2)
	{
		// Se ordena por tiempo de llegada y por pid
		if (arr1[a].arrival < arr2[b].arrival ||
			(arr1[a].arrival == arr2[b].arrival && arr1[a].pid < arr2[b].pid))
		{
			arr[c] = arr1[a];
			++a;
		}
		else
		{
			arr[c] = arr2[b];
			++b;
		}

		++c;
	}

	while (a < size1)
	{
		arr[c] = arr1[a];
		++a;
		++c;
	}

	while (b < size2)
	{
		arr[c] = arr2[b];
		++b;
		++c;
	}

	free(arr1);
	free(arr2);
}

struct process filtrarInput(char *linea, pid_t pidCount)
{
	struct process temp;
	int arr, prio, cpu, p, s, m, d;
	sscanf(linea, "%d, %d, %d, %d, %d, %d, %d", &arr, &prio, &cpu, &p, &s, &m, &d);
	temp.pid = pidCount;
	temp.arrival = arr;
	temp.priority = prio;
	temp.procTime = cpu;
	temp.printer = p;
	temp.scanner = s;
	temp.modem = m;
	temp.dvd = d;
	temp.quantum = 0;

	return temp;
}

int main(int argc, char *argv[])
{
	pthread_t t;
	pthread_t t2;
	struct process temp;
	char *nombreArchivo = argv[1]; //Se toma el nombre del archivo del argumento usado al ejecutar rrobin

	sem_init(&ready, 0, 0);
	sem_init(&blockedSem, 0, 0);
	sem_init(&dispatch, 0, 0);

	bit = 1;

	// El primer segundo es de BEGIN
	seconds = 1;
	// Inicializando iterador y tamaño
	actual = 0;
	n = 0;
	nInBeforeArrival = 0;
	// Placeholder antes de tener el primer proceso
	priority = -1;

	SuperColaInit(&jobList); // Se inicializa la cola retroalimentada

	FILE *i = fopen(nombreArchivo, "r");
	if (!i)
		return EXIT_FAILURE;
	char buffer[256];
	pid_t pidCount = 0;
	while (fgets(buffer, sizeof(buffer), i)) //Se toma una linea del archivo
	{
		if (strlen(buffer) > 1)//Si no es el final
		{
			temp = filtrarInput(buffer, pidCount++); //Se guarda la información del nuevo proceso y se asigna su pid

			if (temp.printer > maxInstanceResources[0])
			{
				printf("El procesos %d rechazado por falta de recursos \n", temp.pid);
				continue;
			}
			if (temp.scanner > maxInstanceResources[1])
			{
				printf("El procesos %d rechazado por falta de recursos \n", temp.pid);
				continue;
			}
			if (temp.modem > maxInstanceResources[2])
			{
				printf("El procesos %d rechazado por falta de recursos \n", temp.pid);
				continue;
			}
			if (temp.dvd > maxInstanceResources[3])
			{
				printf("El procesos %d rechazado por falta de recursos \n", temp.pid);
				continue;
			} //Se rechazan procesos que piden más recursos de los disponibles

			if (temp.arrival > 0)
			{
				beforeArrival[n] = temp; //Se guardan los procesos que llegarán más adelante en la ejecución
				++n;
				++nInBeforeArrival;
			}
			else
			{ // Sólo se meten a la cola los procesos que llegan al segundo 0
				if (canTakeResources(temp.printer, temp.scanner, temp.modem, temp.dvd) || temp.priority == 0)
				{

					availableResource[0] -= temp.printer;
					availableResource[1] -= temp.scanner;
					availableResource[2] -= temp.modem;
					availableResource[3] -= temp.dvd;
					SuperColaPush(&jobList, temp); //Si hay recursos suficientes para el proceso se le asignan y se introduce a la cola
				}
				else
				{

					push(&processesBlocked, temp); //De lo contrario se introduce a la cola de bloqueados
				}

			}
		}
	}
	fclose(i);

	file = fopen("[31708119][31703888][31307754].txt", "w+");

	sortByArrival(beforeArrival, n); // Se ordenan los procesos por tiempo de llegada

	pthread_create(&t, NULL, dispatcher, NULL);
	pthread_create(&t2, NULL, blocked, NULL);

	timer();

	pthread_join(t, NULL);
	sem_post(&blockedSem);
	pthread_join(t2, NULL);

	fclose(file);
	SuperColaDestroy(&jobList);
	QueueDestroy(&processesBlocked);
	sem_destroy(&ready);
	sem_destroy(&dispatch); //Se libera toda la memoria asignada

	return 0;
}
