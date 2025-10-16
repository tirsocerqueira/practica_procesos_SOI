#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

int g_var = 42;

int main(void) {
    int fd = open("compartido.txt", O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if (fd < 0) { perror("open"); return 1; }

    int l_var = 100;
    int *h_var = malloc(sizeof *h_var);
    if (!h_var) { perror("malloc"); return 1; }
    *h_var = 7;

    dprintf(fd, "[PADRE] antes de fork\n");

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    const char *who = (pid == 0) ? "HIJO" : "PADRE";

    // Identificadores + PATH (entorno)
    printf("[%s] pid=%d ppid=%d uid=%d euid=%d gid=%d\n",
           who, getpid(), getppid(), getuid(), geteuid(), getgid());
    printf("[%s] PATH=%s\n", who, getenv("PATH") ? getenv("PATH") : "(null)");

    // Direcciones virtuales (deberían coincidir) y valores
    printf("[%s] &g_var=%p &l_var=%p h_var=%p\n",
           who, (void*)&g_var, (void*)&l_var, (void*)h_var);
    printf("[%s] valores: g=%d l=%d *h=%d\n", who, g_var, l_var, *h_var);

    if (pid == 0) {                // HIJO: modifica (copy-on-write)
        g_var++; l_var++; (*h_var)++;
        printf("[%s] tras modificar: g=%d l=%d *h=%d\n", who, g_var, l_var, *h_var);
    }

    // Competición por stdin
    char buf[64];
    printf("[%s] Escribe una palabra: ", who);
    fflush(stdout);
    if (scanf("%63s", buf) == 1) printf("[%s] leí: %s\n", who, buf);
    else                         printf("[%s] scanf fallo/EOF\n", who);
//muestra explicita offset compartido
    off_t off1 = lseek(fd, 0, SEEK_CUR);
    if (off1 == (off_t)-1) perror("lseek antes");

    if (dprintf(fd, "[%s] escribe tras fork (mismo FD)\n", who) < 0)
        perror("dprintf");

    off_t off2 = lseek(fd, 0, SEEK_CUR);
    if (off2 == (off_t)-1) perror("lseek despues");

    printf("[%s] offset antes=%lld  despues=%lld  (+%lld bytes)\n",
        who, (long long)off1, (long long)off2, (long long)(off2 - off1));
    fflush(stdout);


    if (pid == 0) {               // HIJO
        close(fd);
        free(h_var);
        _exit(0);
    } else {                      // PADRE
        // Deja unos segundos para poder ver ambos procesos con `ps` si quieres
        printf("[PADRE] Observa con: ps -o pid,ppid,stat,cmd | grep entregable1\n");
        sleep(3);
        close(fd);
        free(h_var);
        return 0;
    }
}


