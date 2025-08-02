Para ejecutar el proyecto solo hacen falta las siguientes 2 líneas

make

./rrobin nombre

Con nombre siendo el nombre del archivo con la lista de procesos a ejecutar

El proyecto se ejecuta en 3 hilos

processor: Tiene el proceso siendo ejecutado e imprime los EXECUTING

timer: Lleva el tiempo en segundos y sincroniza las llamadas al dispatcher o la acción del proceso

dispatcher: Maneja lo que entra y sale de la SuperCola

Se tiene una SuperCola encargada de introducir al proceso en la cola de su prioridad y sacar siempre el de mayor prioridad

Preferiblemente mantener la comunicación mediante semáforos de forma:
dispatcher<->timer<->processor


Para hacer un cambio de contexto desde el hilo timer sólo hay que hacer

sem_post(&dispatch);

sem_wait(&ready);

Así, lo único que queda hacer es la lógica de los dispositivos de E/S
(En mi opinión debería ejecutarse en el hilo timer)
