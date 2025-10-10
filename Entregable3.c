{\rtf1\ansi\ansicpg1252\cocoartf2759
\cocoatextscaling0\cocoaplatform0{\fonttbl\f0\fswiss\fcharset0 Helvetica;}
{\colortbl;\red255\green255\blue255;}
{\*\expandedcolortbl;;}
\paperw11900\paperh16840\margl1440\margr1440\vieww33700\viewh21100\viewkind0
\pard\tx720\tx1440\tx2160\tx2880\tx3600\tx4320\tx5040\tx5760\tx6480\tx7200\tx7920\tx8640\pardirnatural\partightenfactor0

\f0\fs24 \cf0 #define _XOPEN_SOURCE 700\
#include <stdio.h>\
#include <stdlib.h>\
#include <unistd.h>\
#include <math.h>\
#include <sys/wait.h>\
#include <sys/time.h>\
#include <string.h>\
#include <errno.h>\
#include <fcntl.h>\
#include <sys/types.h>\
#include <sys/stat.h>\
\
#define FILENAME "resultados.txt"\
\
typedef struct \{\
    double sum;\
    long   count;\
\} Agg; //struct para suma y conteo\
\
// ---------- Tiempo en ms ----------\
static long tiempo_ms(void) \{\
    struct timeval tv;\
    gettimeofday(&tv, NULL);\
    return (long)(tv.tv_sec * 1000L + tv.tv_usec / 1000L);\
\}\
\
// ---------- Escritura con bloqueo (evita mezclar l\'edneas) ----------\
static void append_line_locked(const char *line) \{\
    int fd = open(FILENAME, O_WRONLY | O_CREAT | O_APPEND, 0644);\
    if (fd < 0) \{ perror("open"); _exit(1); \}\
\
    struct flock lk;\
    memset(&lk, 0, sizeof(lk));\
    lk.l_type = F_WRLCK;      // bloqueo exclusivo\
    lk.l_whence = SEEK_SET;\
    lk.l_start = 0;\
    lk.l_len = 0;             // toda la longitud\
\
    if (fcntl(fd, F_SETLKW, &lk) == -1) \{ perror("fcntl lock"); close(fd); _exit(1); \}\
\
    size_t n = strlen(line);\
    ssize_t w = write(fd, line, n);\
    if (w < 0 || (size_t)w != n) \{ perror("write"); \}\
\
    lk.l_type = F_UNLCK;\
    (void)fcntl(fd, F_SETLK, &lk);\
    close(fd);\
\}\
\
// ---------- C\'e1lculo por bloque: suma tan(sqrt(i)) y n\'ba t\'e9rminos ----------\
// Usamos suma compensada de Kahan para mayor precisi\'f3n num\'e9rica.\
static Agg calcular_bloque(int start, int end) \{\
    Agg a = \{0.0, 0\};\
    if (end < start) return a;\
\
    double sum = 0.0, c = 0.0; // Kahan\
    for (int i = start; i <= end; ++i) \{\
        double term = tan(sqrt((double)i));\
        double y = term - c;\
        double t = sum + y;\
        c = (t - sum) - y;\
        sum = t;\
        a.count++;\
    \}\
    a.sum = sum;\
    return a;\
\}\
\
// ---------- Formatos de l\'ednea (para identificar qui\'e9n escribe) ----------\
// Hijos 1..P (bloques):   "Proceso <id>: tipo=bloque sum=<...> count=<...> tiempo=<...>ms\\n"\
// Hijo P+1 (agregado):    "Proceso <id>: tipo=agregado media=<...> tiempo=<...>ms\\n"\
// Hijo P+2 (total):       "Proceso <id>: tipo=total    media=<...> tiempo=<...>ms\\n"\
\
static void escribir_linea_bloque(int id, double sum, long count, long tms) \{\
    char buf[256];\
    int n = snprintf(buf, sizeof(buf),\
        "Proceso %d: tipo=bloque sum=%.17g count=%ld tiempo=%ldms\\n",\
        id, sum, count, tms);\
    if (n < 0 || n >= (int)sizeof(buf)) \{ fprintf(stderr, "snprintf overflow\\n"); _exit(1); \}\
    append_line_locked(buf);\
\}\
\
static void escribir_linea_media(int id, const char *tipo, double media, long tms) \{\
    char buf[256];\
    int n = snprintf(buf, sizeof(buf),\
        "Proceso %d: tipo=%s media=%.17g tiempo=%ldms\\n",\
        id, tipo, media, tms);\
    if (n < 0 || n >= (int)sizeof(buf)) \{ fprintf(stderr, "snprintf overflow\\n"); _exit(1); \}\
    append_line_locked(buf);\
\}\
\
// ---------- Hijo P+1: agrega contribuciones de los P bloques ----------\
static void hijo_agregador(int P, int id_agregador) \{\
    long t0 = tiempo_ms();\
\
    FILE *f = fopen(FILENAME, "r");\
    if (!f) \{ perror("fopen agregador"); _exit(1); \}\
\
    double total_sum = 0.0;\
    long total_count = 0;\
    char linea[512];\
\
    while (fgets(linea, sizeof(linea), f)) \{\
        int id;\
        double sum;\
        long count;\
        long tms;\
        // Parseamos solo l\'edneas de tipo=bloque\
        if (sscanf(linea, "Proceso %d: tipo=bloque sum=%lf count=%ld tiempo=%ldms",\
                   &id, &sum, &count, &tms) == 4) \{\
            if (id >= 1 && id <= P) \{\
                total_sum   += sum;\
                total_count += count;\
            \}\
        \}\
    \}\
    fclose(f); //cierra el archivo y sigue con la media para mostrarla por pantalla\
\
    double media_final = (total_count > 0) ? (total_sum / (double)total_count) : 0.0;\
    long t1 = tiempo_ms();\
\
    printf("Hijo %d (agregador): media_global=%.10f\\n", id_agregador, media_final);\
    escribir_linea_media(id_agregador, "agregado", media_final, t1 - t0);\
\}\
\
// ---------- Hijo P+2: c\'e1lculo completo 1..N en paralelo a todos ----------\
static void hijo_total(int N, int id) \{\
    long t0 = tiempo_ms();\
    Agg a = calcular_bloque(1, N);\
    double media = (a.count > 0) ? (a.sum / (double)a.count) : 0.0;\
    long t1 = tiempo_ms();\
\
    printf("Hijo %d (total 1..N): media=%.10f\\n", id, media);\
    escribir_linea_media(id, "total", media, t1 - t0);\
\}\
\
int main(int argc, char *argv[]) \{\
    if (argc != 3) \{\
        fprintf(stderr, "Uso: %s N P\\n", argv[0]);\
        return 1;\
    \}\
    int N = atoi(argv[1]);\
    int P = atoi(argv[2]);\
    if (N < 1 || P < 1) \{\
        fprintf(stderr, "N y P deben ser mayores que 0\\n");\
        return 1;\
    \}\
\
    // Limpia el archivo\
    \{\
        int fd = open(FILENAME, O_WRONLY | O_CREAT | O_TRUNC, 0644);\
        if (fd < 0) \{ perror("open trunc"); return 1; \}\
        close(fd);\
    \}\
\
    pid_t *hijos = (pid_t*)calloc((size_t)(P + 2), sizeof(pid_t));\
    if (!hijos) \{ perror("calloc"); return 1; \}\
\
    int bloque = N / P;\
    int resto  = N % P;\
\
    // --- Lanza P hijos (bloques) en paralelo ---\
    for (int i = 0; i < P; i++) \{\
        hijos[i] = fork();\
        if (hijos[i] < 0) \{ perror("fork"); free(hijos); return 1; \}\
        if (hijos[i] == 0) \{\
            int start = i * bloque + 1;\
            int end   = (i == P - 1) ? ((i + 1) * bloque + resto) : ((i + 1) * bloque);\
\
            long t0 = tiempo_ms();\
            Agg a = calcular_bloque(start, end);\
            long t1 = tiempo_ms();\
\
            double media = (a.count > 0) ? (a.sum / (double)a.count) : 0.0;\
            printf("Hijo %d (bloque %d..%d): media=%.10f\\n", i + 1, start, end, media);\
\
            escribir_linea_bloque(i + 1, a.sum, a.count, t1 - t0);\
            _exit(0);\
        \}\
    \}\
\
    // --- Lanza P+2 en paralelo a todos los anteriores ---\
    hijos[P + 1] = fork();\
    if (hijos[P + 1] < 0) \{ perror("fork P+2"); free(hijos); return 1; \}\
    if (hijos[P + 1] == 0) \{\
        hijo_total(N, P + 2);\
        _exit(0);\
    \}\
\
    // --- El padre espera SOLO a los P hijos de bloques ---\
    for (int i = 0; i < P; ++i) \{\
        int st;\
        (void)waitpid(hijos[i], &st, 0);\
    \}\
\
    // --- Lanzar P+1 (agregador) DESPU\'c9S de que existan las P contribuciones ---\
    hijos[P] = fork();\
    if (hijos[P] < 0) \{ perror("fork P+1"); free(hijos); return 1; \}\
    if (hijos[P] == 0) \{\
        hijo_agregador(P, P + 1);\
        _exit(0);\
    \}\
\
    // --- Esperar a P+1 y P+2 ---\
    for (int i = P; i < P + 2; ++i) \{\
        int st;\
        (void)waitpid(hijos[i], &st, 0);\
    \}\
\
    // --- Lectura final por el PADRE: diferencia y tiempos ---\
    double media_agregado = 0.0, media_total = 0.0;\
    long *tiempos = (long*)calloc((size_t)(P + 2), sizeof(long));\
    if (!tiempos) \{ perror("calloc tiempos"); free(hijos); return 1; \}\
\
    FILE *f = fopen(FILENAME, "r");\
    if (!f) \{ perror("fopen lectura final"); free(tiempos); free(hijos); return 1; \}\
\
    char linea[512];\
    while (fgets(linea, sizeof(linea), f)) \{\
        int id;\
        long tms;\
        // Bloques\
        double sum; long count;\
        if (sscanf(linea, "Proceso %d: tipo=bloque sum=%lf count=%ld tiempo=%ldms",\
                   &id, &sum, &count, &tms) == 4) \{\
            if (id >= 1 && id <= P + 2) tiempos[id - 1] = tms;\
            continue;\
        \}\
        // Agregado\
        double media;\
        if (sscanf(linea, "Proceso %d: tipo=agregado media=%lf tiempo=%ldms",\
                   &id, &media, &tms) == 3) \{\
            if (id == P + 1) media_agregado = media;\
            if (id >= 1 && id <= P + 2) tiempos[id - 1] = tms;\
            continue;\
        \}\
        // Total\
        if (sscanf(linea, "Proceso %d: tipo=total media=%lf tiempo=%ldms",\
                   &id, &media, &tms) == 3) \{\
            if (id == P + 2) media_total = media;\
            if (id >= 1 && id <= P + 2) tiempos[id - 1] = tms;\
            continue;\
        \}\
    \}\
    fclose(f);\
\
    printf("\\nDiferencia entre medias (P+1 vs P+2): %.10f\\n",\
           fabs(media_agregado - media_total));\
\
    for (int i = 0; i < P + 2; ++i) \{\
        printf("Tiempo hijo %d: %ldms\\n", i + 1, tiempos[i]);\
    \}\
\
    free(tiempos);\
    free(hijos);\
    return 0;\
\}	}