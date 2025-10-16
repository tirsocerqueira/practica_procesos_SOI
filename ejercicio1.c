#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

// Variable global
// Se utiliza para demostrar cómo fork copia el espacio de memoria
int g_var = 42;

int main(void) {
    // ───────────── Apertura de fichero ─────────────
    // Abrimos el fichero antes del fork para demostrar que el descriptor
    // se comparte entre padre e hijo
    // O_CREAT  -> crea el fichero si no existe
    // O_TRUNC  -> trunca el fichero a tamaño 0 si ya existe
    // O_WRONLY -> apertura en modo escritura
    int fd = open("compartido.txt", O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if (fd < 0) {
        perror("Error al abrir el archivo");
        return 1;
    }

    // ───────────── Declaración de variables ─────────────
    // Variable local en stack
    int l_var = 100;                    
    // Variable dinámica en heap
    int *h_var = malloc(sizeof *h_var); 
    if (!h_var) {
        perror("Error en malloc");
        return 1;
    }
    *h_var = 7;

    // Escritura inicial en el fichero antes del fork
    // Demuestra que el padre puede escribir antes de crear el hijo
    dprintf(fd, "[PADRE] antes de fork\n");

    // ───────────── Creación del proceso hijo ─────────────
    pid_t pid = fork();

    if (pid < 0) {
        perror("Error en fork");
        return 1;
    }

    // ───────────── BLOQUE DEL HIJO ─────────────
    if (pid == 0) {
        const char *who = "HIJO";

        // Información identificativa del proceso hijo
        printf("\n[%s] Información del proceso:\n", who);
        printf("[%s] pid=%d, ppid=%d, uid=%d, euid=%d, gid=%d\n",
               who, getpid(), getppid(), getuid(), geteuid(), getgid());

        // Mostramos el PATH heredado del padre
        const char *path = getenv("PATH");
        printf("[%s] PATH=%s\n", who, path ? path : "(no definido)");

        // Direcciones virtuales de memoria
        // Virtualmente coinciden con las del padre
        printf("\n[%s] Direcciones virtuales:\n", who);
        printf("[%s] &g_var = %p (global)\n",  who, (void*)&g_var);
        printf("[%s] &l_var = %p (local)\n",   who, (void*)&l_var);
        printf("[%s] h_var  = %p (dinámica)\n", who, (void*)h_var);

        // Valores antes de modificar variables
        printf("\n[%s] Valores antes de modificar:\n", who);
        printf("[%s] g_var = %d, l_var = %d, *h_var = %d\n", who, g_var, l_var, *h_var);

        // Modificamos variables en el hijo
        // Esto permite demostrar que la copia es independiente (copy-on-write)
        g_var++;
        l_var++;
        (*h_var)++;

        // Valores después de modificar
        printf("\n[%s] Valores tras modificar:\n", who);
        printf("[%s] g_var = %d, l_var = %d, *h_var = %d\n", who, g_var, l_var, *h_var);

        // Lectura desde stdin
        // Demuestra competencia entre padre e hijo por el acceso al teclado
        char buf[64];
        printf("\n[%s] Escribe una palabra: ", who);
        fflush(stdout);
        if (scanf("%63s", buf) == 1)
            printf("[%s] Leí: %s\n", who, buf);
        else
            printf("[%s] Error o EOF en scanf\n", who);

        // Escritura en el fichero compartido usando dprintf
        // dprintf evita problemas de buffers de stdio duplicados tras fork
        dprintf(fd, "[%s] escribe tras fork (mismo FD)\n", who);
        // Guardamos el estado final de las variables del hijo en el fichero
        dprintf(fd, "[%s] Valores finales -> g_var=%d, l_var=%d, *h_var=%d\n",
                who, g_var, l_var, *h_var);

        // Liberamos memoria y cerramos descriptor de fichero
        close(fd);
        free(h_var);
        // Salimos sin pasar por handlers de salida duplicados
        _exit(0);
    }

    // ───────────── BLOQUE DEL PADRE ─────────────
    else {
        const char *who = "PADRE";

        // Información identificativa del proceso padre
        printf("\n[%s] Información del proceso:\n", who);
        printf("[%s] pid=%d, ppid=%d, uid=%d, euid=%d, gid=%d\n",
               who, getpid(), getppid(), getuid(), geteuid(), getgid());

        // PATH heredado
        const char *path = getenv("PATH");
        printf("[%s] PATH=%s\n", who, path ? path : "(no definido)");

        // Direcciones virtuales de memoria
        printf("\n[%s] Direcciones virtuales:\n", who);
        printf("[%s] &g_var = %p (global)\n",  who, (void*)&g_var);
        printf("[%s] &l_var = %p (local)\n",   who, (void*)&l_var);
        printf("[%s] h_var  = %p (dinámica)\n", who, (void*)h_var);

        // Valores antes de modificar variables
        printf("\n[%s] Valores antes de modificar:\n", who);
        printf("[%s] g_var = %d, l_var = %d, *h_var = %d\n", who, g_var, l_var, *h_var);

        // Lectura desde stdin
        char buf[64];
        printf("\n[%s] Escribe una palabra: ", who);
        fflush(stdout);
        if (scanf("%63s", buf) == 1)
            printf("[%s] Leí: %s\n", who, buf);
        else
            printf("[%s] Error o EOF en scanf\n", who);

        // Escritura en el fichero compartido
        dprintf(fd, "[%s] escribe tras fork (mismo FD)\n", who);
        // Guardamos el estado final de las variables del padre en el fichero
        dprintf(fd, "[%s] Valores finales -> g_var=%d, l_var=%d, *h_var=%d\n",
                who, g_var, l_var, *h_var);

        // Liberación de recursos
        close(fd);
        free(h_var);

        // Fin del proceso padre
        return 0;
    }
}
