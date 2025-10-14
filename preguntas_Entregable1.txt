SISTEMAS OPERATIVOS I - PRÁCTICA 2: GESTIÓN DE PROCESOS
Respuestas: Diego Fernández Márquez

A. CREACIÓN DE PROCESOS Y API POSIX
1) ¿Qué devuelve fork() en el padre y qué devuelve en el hijo?
En el padre devuelve el PID del hijo (>0). En el hijo devuelve 0. Se diferencia con if (pid == 0) para el hijo y else para el padre.

2) ¿Qué ocurre si fork() falla?
Devuelve -1. Se detecta con if (pid < 0) y se maneja con perror("fork") y salida con error.

3) Explica la diferencia entre PID y PPID. ¿Qué verías en getppid() si el padre termina antes que el hijo?
PID es el identificador del proceso. PPID es el identificador del proceso padre. Si el padre muere antes, el PPID del hijo pasa a ser 1 (adoptado por init/systemd).

4) ¿Por qué pones _POSIX_C_SOURCE 200809L al principio del archivo?
Para garantizar que los encabezados expongan funciones estándar POSIX (getpid, dprintf, etc.) y evitar funciones no estándar.

5) ¿Qué diferencias habría si en vez de procesos usaras hilos (threads)?
Los hilos comparten el mismo espacio de direcciones; no hay copy-on-write. Habría que sincronizar accesos a memoria y E/S con mutex.

B. IDENTIDAD DEL PROCESO Y ENTORNO
6) ¿Para qué sirve cada una de estas llamadas: getpid, getppid, getuid, geteuid, getgid? Pon un caso donde UID ≠ EUID.
getpid: PID del proceso. getppid: PID del padre. getuid: UID real. geteuid: UID efectivo (permisos actuales). getgid: GID real. Caso UID≠EUID: binarios setuid root (p.ej. /bin/passwd).

7) ¿Cómo hereda el hijo el entorno del padre? ¿Por qué getenv("PATH") muestra algo en ambos?
El hijo recibe una copia del entorno del padre. Por eso getenv("PATH") devuelve lo mismo en ambos procesos.

8) ¿Qué pasaría si en el hijo haces setenv("PATH", "...")? ¿Se ve en el padre? ¿Por qué?
No. Cada proceso tiene su propia copia del entorno, así que los cambios del hijo no afectan al padre.

C. MEMORIA: DIRECCIONES VIRTUALES, COW Y SEGMENTOS
9) ¿Por qué coinciden las direcciones virtuales en padre e hijo tras fork()?
Porque fork duplica el espacio de direcciones virtuales. Ambos comienzan con el mismo mapa de memoria.

10) Explica copy-on-write (COW) y en qué momento se materializa (se copian) las páginas de memoria.
COW permite compartir páginas entre padre e hijo hasta que uno escribe en ellas. La copia real ocurre al primer write en esa página.

11) ¿Cómo demuestras que las direcciones virtuales coinciden pero los valores son distintos?
Imprimiendo &g_var, &l_var y h_var (mismas direcciones) y modificando valores solo en el hijo. El padre mantiene los valores originales.

12) Distingue los segmentos donde viven g_var, l_var y *h_var.
g_var: segmento de datos (.data). l_var: pila (stack). *h_var: montículo (heap).

13) ¿Qué cambiaría si usas memoria compartida (mmap con MAP_SHARED o SHM)?
Los cambios del hijo serían visibles en el padre, porque ambos accederían a la misma región física.

14) ¿La ASLR afecta a las direcciones virtuales justo tras fork()?
No. ASLR se aplica al ejecutar (exec). En fork el hijo hereda el mismo layout virtual del padre.

D. ENTRADA/SALIDA Y COMPETENCIA POR STDIN
15) ¿Por qué ambos procesos compiten por stdin al hacer scanf?
Porque comparten el mismo terminal. Ambos intentan leer del mismo flujo de entrada, y uno puede quedarse bloqueado o fallar.

16) ¿Para qué haces fflush(stdout) antes del scanf? Explica line buffering.
stdout en terminal es line-buffered; al duplicarse el buffer tras fork, fflush evita que los mensajes se impriman dos veces o desordenados.

17) ¿Qué ocurre si el usuario introduce solo una palabra? ¿Y si envía EOF (Ctrl-D)?
Con una palabra scanf devuelve 1 y lee correctamente. Si recibe EOF devuelve un valor negativo (fallo o EOF).

18) ¿Cómo sincronizarías el acceso a stdin para evitar competencia?
Podría usarse waitpid para que solo uno lea, cerrar stdin en uno de los procesos (close(0)), o usar pipes/señales.

E. FICHEROS Y DESCRIPTORES COMPARTIDOS
19) ¿Por qué abres el fichero antes de fork()? ¿Qué se hereda exactamente?
Porque el descriptor abierto se hereda. Ambos procesos comparten la misma open file description (offset y flags).

20) ¿Cómo demostrarías que comparten offset?
Usando lseek(fd, 0, SEEK_CUR) antes y después de escribir. Ambos verían el mismo desplazamiento actualizado.

21) ¿Tus escrituras con dprintf(fd, ...) son atómicas? ¿Qué pasa si las líneas son largas? ¿Cambiaría algo usando O_APPEND?
Cada write es atómico individualmente, pero varias llamadas pueden entrelazarse. O_APPEND hace el posicionamiento y escritura atómicos.

22) ¿Qué pasa si en el hijo cierras con close(fd) y el padre sigue escribiendo? ¿Y al revés?
El descriptor se cierra solo en ese proceso. Mientras haya otra referencia abierta, el fichero sigue accesible.

23) ¿Para qué usarías O_CLOEXEC si luego llamas a exec()?
Para que el descriptor se cierre automáticamente al hacer exec, evitando fugas de descriptores abiertos.

F. ORDEN, SINCRONIZACIÓN Y FINALIZACIÓN
24) ¿Por qué usar waitpid(pid, &st, 0)?
Para evitar procesos zombie y sincronizar la finalización del hijo.

25) ¿En tu diseño podría quedar un zombie?
Sí, si el padre termina sin hacer waitpid. Se evita con waitpid o manejadores SIGCHLD.

26) ¿Por qué en el hijo usas _exit(0) y no exit(0)?
_exit() termina sin vaciar buffers duplicados de stdio. exit() podría causar duplicación de salidas al heredarse buffers.

G. VERIFICACIÓN Y DEPURACIÓN
27) ¿Qué comandos usarías para observar la relación padre-hijo?
ps -o pid,ppid,stat,cmd | grep programa
pstree -p
strace -f ./entregable1
lsof -p <PID>

28) ¿Cómo demostrarías que el padre no ve los cambios hechos por el hijo?
Tras waitpid, imprimir nuevamente g_var, l_var y *h_var en el padre; mantienen sus valores originales.

29) (Extra) ¿Qué añadirías para demostrar el offset compartido?
Imprimir lseek antes y después de cada dprintf en ambos procesos para comparar desplazamientos y ver que avanzan juntos.
