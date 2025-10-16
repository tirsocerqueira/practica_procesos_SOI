#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/time.h>

#define FILENAME "resultados.txt"  // Nombre del archivo compartido donde se guardan los resultados

// ---------- Función: calcular_bloque ----------
// Calcula la suma de tan(sqrt(i)) para un bloque de números [start..end]
// y devuelve también el conteo de elementos en el bloque.
void calcular_bloque(int start, int end, double *sum, long *count) {
    *sum = 0.0;   // Inicializa la suma
    *count = 0;   // Inicializa el contador de elementos
    for (int i = start; i <= end; i++) {
        *sum += tan(sqrt((double)i)); // Convierte a double y calcula tan(sqrt(i))
        (*count)++;                   // Incrementa contador de elementos procesados
    }
}

// ---------- Función: append_line ----------
// Añade una línea al archivo compartido FILENAME.
// Se usa para que todos los procesos escriban resultados en el mismo archivo.
void append_line(const char *line) {
    FILE *f = fopen(FILENAME, "a"); // Abrir en modo append
    if (!f) { 
        perror("fopen"); // Manejo de errores
        _exit(1);        // Salida inmediata en proceso hijo si hay error
    }
    fputs(line, f); // Escribe la línea en el archivo
    fclose(f);      // Cierra el archivo
}

// ---------- Función: hijo_agregador ----------
// Hijo P+1 que agrega los resultados de todos los P hijos de bloques
// y calcula la media global. También mide el tiempo de ejecución.
void hijo_agregador(int P, int id_agregador) {
    struct timeval t0, t1;
    gettimeofday(&t0, NULL); // Marca tiempo inicial

    FILE *f = fopen(FILENAME, "r"); // Abre el archivo para leer resultados de bloques
    if (!f) { perror("fopen agregador"); _exit(1); }

    double total_sum = 0.0; // Suma acumulada de todos los bloques
    long total_count = 0;   // Conteo acumulado de elementos
    char linea[512];        // Buffer para leer cada línea del archivo

    // Leer línea por línea
    while (fgets(linea, sizeof(linea), f)) {
        int id;
        double sum;
        long count;
        // Filtra solo las líneas de tipo "bloque"
        if (sscanf(linea, "Proceso %d: tipo=bloque sum=%lf count=%ld",
                   &id, &sum, &count) == 3) {
            if (id >= 1 && id <= P) { // Solo considera hijos de bloques
                total_sum += sum;     // Acumula la suma parcial
                total_count += count; // Acumula el conteo
            }
        }
    }
    fclose(f);

    double media_final = (total_count > 0) ? (total_sum / (double)total_count) : 0.0; // Calcula la media global

    gettimeofday(&t1, NULL); // Marca tiempo final
    double tiempo = (t1.tv_sec - t0.tv_sec) + (t1.tv_usec - t0.tv_usec)/1e6; // Calcula tiempo en segundos

    // Muestra por consola
    printf("Hijo %d (agregador): media_global=%.10f, tiempo=%.6f s\n",
           id_agregador, media_final, tiempo);

    // Guarda en el archivo compartido
    char buf[256];
    snprintf(buf, sizeof(buf),
             "Proceso %d: tipo=agregado media=%.17g tiempo=%.6f\n",
             id_agregador, media_final, tiempo);
    append_line(buf);
}

// ---------- Función: hijo_total ----------
// Hijo P+2 que calcula directamente la media de todos los N números.
// También mide su tiempo de ejecución y guarda resultados en el archivo.
void hijo_total(int N, int id) {
    struct timeval t0, t1;
    gettimeofday(&t0, NULL); // Tiempo inicial

    double sum;
    long count;
    calcular_bloque(1, N, &sum, &count); // Calcula suma y conteo de 1..N
    double media = (count > 0) ? (sum / (double)count) : 0.0;

    gettimeofday(&t1, NULL); // Tiempo final
    double tiempo = (t1.tv_sec - t0.tv_sec) + (t1.tv_usec - t0.tv_usec)/1e6;

    printf("Hijo %d (total 1..N): media=%.10f, tiempo=%.6f s\n", id, media, tiempo);

    // Guardar en archivo compartido
    char buf[256];
    snprintf(buf, sizeof(buf),
             "Proceso %d: tipo=total media=%.17g tiempo=%.6f\n",
             id, media, tiempo);
    append_line(buf);
}

// ---------- Función: hijo_bloque ----------
// Cada hijo de bloque calcula la media de su bloque de números.
// Se mide tiempo de ejecución y se guarda resultado en archivo.
void hijo_bloque(int start, int end, int id) {
    struct timeval t0, t1;
    gettimeofday(&t0, NULL);

    double sum;
    long count;
    calcular_bloque(start, end, &sum, &count);
    double media = (count > 0) ? (sum / (double)count) : 0.0;

    gettimeofday(&t1, NULL);
    double tiempo = (t1.tv_sec - t0.tv_sec) + (t1.tv_usec - t0.tv_usec)/1e6;

    printf("Hijo %d (bloque %d..%d): media=%.10f, tiempo=%.6f s\n",
           id, start, end, media, tiempo);

    // Guarda en archivo compartido, identificando proceso y bloque
    char buf[256];
    snprintf(buf, sizeof(buf),
             "Proceso %d: tipo=bloque sum=%.17g count=%ld tiempo=%.6f\n",
             id, sum, count, tiempo);
    append_line(buf);
}

// ---------- Función principal ----------
int main(int argc, char *argv[]) {
    // Validación de argumentos
    if (argc != 3) {
        fprintf(stderr, "Uso: %s N P\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]); // Rango máximo
    int P = atoi(argv[2]); // Número de hijos de bloques
    if (N < 1 || P < 1) {
        fprintf(stderr, "N y P deben ser mayores que 0\n");
        return 1;
    }

    // Limpiar archivo de resultados
    FILE *f = fopen(FILENAME, "w");
    if (!f) { perror("fopen"); return 1; }
    fclose(f);

    pid_t *hijos = (pid_t*)calloc(P + 2, sizeof(pid_t)); // Array para P hijos de bloques + agregador + total
    if (!hijos) { perror("calloc"); return 1; }

    int bloque = N / P; // Tamaño de cada bloque
    int resto  = N % P; // Posible resto en el último bloque

    // --- Lanza P hijos de bloques ---
    for (int i = 0; i < P; i++) {
        hijos[i] = fork();
        if (hijos[i] < 0) { perror("fork"); free(hijos); return 1; }
        if (hijos[i] == 0) { // Código del hijo
            int start = i * bloque + 1;
            int end = (i == P - 1) ? ((i + 1) * bloque + resto) : ((i + 1) * bloque);
            hijo_bloque(start, end, i + 1);
            _exit(0); // Termina hijo
        }
    }

    // --- Lanza hijo total (P+2) ---
    hijos[P + 1] = fork();
    if (hijos[P + 1] < 0) { perror("fork total"); free(hijos); return 1; }
    if (hijos[P + 1] == 0) {
        hijo_total(N, P + 2);
        _exit(0);
    }

    // --- Espera a P hijos de bloques ---
    for (int i = 0; i < P; i++) waitpid(hijos[i], NULL, 0);

    // --- Lanza hijo agregador (P+1) ---
    hijos[P] = fork();
    if (hijos[P] < 0) { perror("fork agregador"); free(hijos); return 1; }
    if (hijos[P] == 0) {
        hijo_agregador(P, P + 1);
        _exit(0);
    }

    // --- Espera a hijo agregador y total ---
    waitpid(hijos[P], NULL, 0);
    waitpid(hijos[P + 1], NULL, 0);

    // --- Calcula diferencia entre agregador y total ---
    double media_agregado = 0.0, media_total = 0.0;
    char linea[512];
    f = fopen(FILENAME, "r");
    if (!f) { perror("fopen lectura final"); free(hijos); return 1; }
    while (fgets(linea, sizeof(linea), f)) {
        int id;
        double media, tiempo;
        // Extrae media del agregador
        if (sscanf(linea, "Proceso %d: tipo=agregado media=%lf tiempo=%lf", &id, &media, &tiempo) == 3)
            media_agregado = media;
        // Extrae media del total
        if (sscanf(linea, "Proceso %d: tipo=total media=%lf tiempo=%lf", &id, &media, &tiempo) == 3)
            media_total = media;
    }
    fclose(f);

    // Muestra la diferencia
    printf("Diferencia entre agregador y total: %.17g\n", media_agregado - media_total);

    free(hijos);
    return 0;
}
