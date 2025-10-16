#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    pid_t pid1, pid2;
    int status;

    /////////////////////////
    // PRIMER FORK - HIJO ZOMBIE 
    // Es zombie mientras el Proceso padre no recoja su estado y lo elimine de la tabla de procesos
    // Basta con que el padre viva más tiempo que este proceso => sleep en el padre
    /////////////////////////
    pid1 = fork();

    // Si PID -1 es un error
    if (pid1 < 0) {
        perror("Error al crear el primer hijo");
        return 1;
    }

    // Lógica del primer hijo
    if (pid1 == 0) {
        // Primer hijo: termina rápido para convertirse en zombie
        printf("Hijo 1 (zombie) - PID: %d, PPID: %d\n", getpid(), getppid());
        // Se le pone el 7 para comprobarlo en la macro definida posteriormente
        exit(7);  // Código de salida que luego leerá el padre
    }

    ///////////////////////////
    // SEGUNDO FORK - HIJO HUÉRFANO
    // Es huérfano ya que sobrevive al fin de ejecución del proceso padre
    // Sleep mayor al padre y el padre no sincroniza a su hijo con wait
    ///////////////////////////
    pid2 = fork();

    if (pid2 < 0) {
        perror("Error al crear el segundo hijo");
        return 1;
    }

    if (pid2 == 0) {
        // Segundo hijo: vive más tiempo, se convierte en huérfano
        printf("Hijo 2 (huérfano) antes de sleep - PID: %d, PPID: %d\n", getpid(), getppid());
        sleep(40);  // Tiempo para que el padre muera y quede huérfano

        // Mostrar nuevo PPID tras quedar huérfano (adoptado por init/systemd)
        printf("Hijo 2 (huérfano) después de sleep - PID: %d, nuevo PPID: %d\n", getpid(), getppid());

        // Sustituye su imagen con `ls -l` usando exec
        execl("/bin/ls", "ls", "-l", NULL);

        // Este printf NUNCA debe ejecutarse si execl tiene éxito
        printf("Este mensaje NO debería mostrarse si execl funciona\n");

        exit(1); // Solo si execl falla
    }

    ///////////////////////////
    // PADRE
    // Debe dormir más que el tiempo de acción del primer proceso y estar activo menos que el segundo
    ///////////////////////////
    printf("Padre - PID: %d\n", getpid());

    // Espera para que el primer hijo se convierta en zombie
    sleep(20);

    // Recolecta el estado del hijo zombie
    pid_t zombie_pid = wait(&status);
    if (WIFEXITED(status)) {
        printf("Padre: Hijo con PID %d terminó con código %d\n", zombie_pid, WEXITSTATUS(status));
    } else {
        printf("Padre: Hijo con PID %d terminó de forma anormal\n", zombie_pid);
    }

    // El padre termina antes que el segundo hijo, lo que lo vuelve huérfano
    printf("Padre finaliza, el hijo 2 se volverá huérfano\n");
    exit(0);
}
