#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <time.h>

/* Ventanas de tiempo (ajústalas si quieres observar con más calma) */
enum {
    SLEEP_PADRE_ANTES_WAIT   = 12,   /* tiempo para que hijo1 quede zombie visible */
    SLEEP_HIJO2_LARGO        = 20,  /* asegura que hijo2 sobreviva al padre -> huérfano */
};



int main(void) {
    pid_t pid1, pid2;

    printf("[PADRE %d] Iniciando. Creo Hijo1 (zombie) y Hijo2 (huérfano).\n", getpid());
    fflush(stdout);

    /* --- Primer hijo: terminar rápido -> zombie hasta que el padre haga waitpid --- */
    pid1 = fork();
    if (pid1 < 0) {
        perror("fork hijo1");
        return 1;
    }
    if (pid1 == 0) {
        /* HIJO 1 */
        printf("[HIJO1 %d, padre=%d] Voy a terminar YA (exit(7)) para convertirme en zombie.\n",
               getpid(), getppid());
        fflush(stdout);
        /* sale con código 7 para comprobar WEXITSTATUS en el padre */
        _exit(7);
    }

    /* --- Segundo hijo: vive más que el padre -> huérfano --- */
    pid2 = fork();
    if (pid2 < 0) {
        perror("fork hijo2");
        return 1;
    }
    if (pid2 == 0) {
        /* HIJO 2 */
        printf("[HIJO2 %d, padre=%d] Empiezo a dormir %ds. "
               "Mi padre debería morir antes y me volveré huérfano.\n",
               getpid(), getppid(), SLEEP_HIJO2_LARGO);
        fflush(stdout);

        sleep(SLEEP_HIJO2_LARGO);

        /* Tras dormir, si el padre ya murió, mi PPID habrá cambiado (a 1/systemd) */
        printf("[HIJO2 %d] Me desperté. Mi PPID AHORA es %d (esperado: 1 o PID de systemd).\n",
               getpid(), getppid());
        fflush(stdout);

        /* Requisito: al final del segundo hijo, cambiar imagen con exec* y luego un printf.
           El printf NO debe ejecutarse si exec tiene éxito. */
        printf("[HIJO2 %d] Voy a ejecutar exec* (execlp(\"/bin/echo\", ...)).\n", getpid());
        fflush(stdout);

        execlp("echo", "echo",
               "HIJO2: ejecutando /bin/echo mediante exec*. Si ves esto, exec funcionó.",
               (char *)NULL);

        /* Si llega aquí, exec falló */
        perror("[HIJO2] exec* ha fallado");
        _exit(127);
    }

    /* --- PADRE: esperar un poco para observar el zombie del Hijo1 --- */
    info_ps_hint("Tras crear ambos hijos. Ahora Hijo1 debe estar Z (zombie) y Hijo2 vivo.");
    printf("[PADRE %d] Duermo %ds para que puedas ver al Hijo1 como zombie (estado Z)...\n",
           getpid(), SLEEP_PADRE_ANTES_WAIT);
    fflush(stdout);
    sleep(SLEEP_PADRE_ANTES_WAIT);

    /* --- PADRE: recoger al hijo1 con waitpid e imprimir WEXITSTATUS --- */
    int st = 0;
    pid_t w = waitpid(pid1, &st, 0);
    if (w == -1) {
        perror("[PADRE] waitpid hijo1");
    } else {
        if (WIFEXITED(st)) {
            int code = WEXITSTATUS(st);
            printf("[PADRE %d] waitpid(hijo1=%d) OK. WIFEXITED=1, WEXITSTATUS=%d\n",
                   getpid(), pid1, code);
        } else if (WIFSIGNALED(st)) {
            printf("[PADRE %d] waitpid(hijo1=%d) OK. Hijo terminó por señal %d\n",
                   getpid(), pid1, WTERMSIG(st));
        } else {
            printf("[PADRE %d] waitpid(hijo1=%d) OK. Estado no convencional.\n",
                   getpid(), pid1);
        }
    }
    fflush(stdout);

    info_ps_hint("Tras waitpid(hijo1). El zombie debe haber desaparecido. Hijo2 sigue vivo.");

    /* --- PADRE: terminar AHORA para dejar huérfano al hijo2 --- */
    printf("[PADRE %d] Termino ahora para que Hijo2 quede huérfano y sea adoptado por init/systemd.\n",
           getpid());
    fflush(stdout);

    /* No esperamos a hijo2 a propósito (para que quede huérfano). */
    return 0;
}
