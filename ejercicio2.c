#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main() {

    // Se usa el tipo pid_t en vez de int porque garantiza portabilidad a otro sistemas UNIX
    pid_t pid1, pid2;

    /////////////////////////
    // PRIMER FORK 
    /////////////////////////
    // Crea el primer hijo, el hijo que será zombie
    /* Condición para que un hijo se transforme en zombie: 
        - El proceso termina su ejecución y su padre no utiliza wait/waitpid para recoger la salida del proceso
        => Para forzar esta situación el hijo debe terminar inmediatamente (exit) y el padre ejecutar una llamada que lo obligue a sobrevivir
        por ejemplo un sleep

        Para observar que ha muerto => /bin/ps -p y número de proceso
        O bien:
                                    => /bin/ps -p número proces -o pid,ppid,state (código de proceso,ódigo de proceso padre, estado)
                                    => si estado = defunct en el primer caso o Z+ en el segundo => es zombie
    */
    pid1 = fork();

    // Si el PID del primer hijo es menor que cero indica que ha tenido un error en la ejecución
    if (pid1 < 0) {
        perror("Error al crear el primer hijo");
        return 1;
    }

    // Si devuelve cero se ha creado el hijo correctamente
    if (pid1 == 0) {
        // Entramos en el primer hijo
        printf("Soy el primer hijo, mi PID es %d, el PID de mi padre es %d\n", getpid(), getppid());
        exit(0);
    } else {
        /////////////////////////
        // SEGUNDO FORK 
        /////////////////////////
        // Padre crea al segundo hijo, el hijo que sea huérfano
        pid2 = fork();

        if (pid2 < 0) {
            perror("Error al crear el segundo hijo");
            return 1;
        }

        // Si devuelve cero se ha creado el hijo correctamente
        if (pid2 == 0) {
            // Entramos en el segundo hijo
            printf("Soy el segundo hijo antes de que muera mi padre, mi PID es %d, el PID de mi padre es %d\n", getpid(), getppid());
            // Se debe dormir al hijo por encima del sleep del padre para forzar que sobreviva al padre
            sleep(30);
            printf("Soy el segundo hijo después de que muera mi padre, mi PID es %d, el PID de mi padre es %d\n", getpid(), getppid());
            
        } else {
            // Proceso padre
            printf("Soy el padre, mi PID es %d\n", getpid());
            // Espera para forzar que el padre sobreviva al primer hijo
            sleep(20);

            // Después de los 40 segundos, sale para mostrar que el segundo hijo se queda huérfano y que lo adopta otro proceso padre
            exit(0);
        }
    }

    return 0;
}
