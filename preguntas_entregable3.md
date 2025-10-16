## PREGUNTAS PRÁCTICA 3 SOI — Media de tan(sqrt(i)) con P+2 procesos

1) ¿Por qué divides el trabajo en P bloques casi iguales y el último añade el resto N%P?
   Para balancear carga: cada hijo procesa N/P elementos y el último absorbe el sobrante, evitando perder términos.

2) ¿Qué ocurriría si repartieses el resto entre los primeros hijos en lugar del último?
   También balancea, pero complica el cálculo de rangos; funcionalmente equivalente si cubres 1..N sin solaparse.

3) ¿Qué pasa si P > N?
   Habrá hijos con bloques vacíos; deben terminar rápidamente sin escribir contribuciones (count=0) para no contaminar el agregado.

4) ¿Por qué i inicia en 1 y no en 0?
   Para evitar tan(sqrt(0))=tan(0)=0 no es problemático, pero se eligió 1..N por el enunciado; incluir 0 no cambia la corrección.

5) ¿Por qué usas Kahan en la suma del bloque?
   Reduce el error de redondeo al acumular muchos términos con magnitudes muy distintas; mejora estabilidad frente a sumas simples.

6) ¿Qué pasaría si quitas Kahan?
   La media seguiría siendo “correcta” pero con mayor error numérico, especialmente para N grandes o términos extremos.

7) ¿Por qué escoges double y no float o long double?
   double ofrece buen compromiso precisión/rendimiento; float pierde precisión; long double aumenta coste y no siempre aporta en HW común.

8) ¿Qué riesgos hay con tan(√i)?
   tan(x) crece cerca de π/2 + kπ; aunque √i no alcance exactamente esos puntos, puede generar valores muy grandes y afectar la suma.

9) ¿Cómo mitigas valores extremos de tan?
   No se recorta en esta versión; Kahan ayuda en la suma. Opcionalmente se podría detectar/limitar outliers o usar sumas por bloques más pequeños.

10) ¿Por qué bloqueas el archivo con fcntl(F_SETLKW) al escribir?
    Para evitar intercalado de líneas entre procesos y garantizar atomicidad lógica de cada registro.

11) ¿Qué pasaría si solo usaras O_APPEND sin locking?
    El offset sería seguro, pero varias writes podrían entremezclarse si una línea supera la granularidad de write; perderías integridad de registros.

12) ¿Por qué bloqueas todo el archivo y no rangos por proceso?
    Simplifica la implementación; el coste del lock por línea es aceptable. Rangos exigirían prefijar offsets o usar mmap/formatos binarios.

13) ¿Qué formato de línea eliges y por qué?
    “Proceso <id>: tipo=… …” estandariza parseo con sscanf y permite distinguir bloques, agregado (P+1) y total (P+2).

14) ¿Qué problemas puede tener el parseo con sscanf?
    Cambios de formato/locale romperían el parseo; alternativa robusta: CSV/JSON o binario con tamaños fijos.

15) ¿Por qué el agregador (P+1) se lanza después de esperar a los P bloques?
    Para asegurar que todas las contribuciones estén ya persistidas; evita race de lectura parcial o ausencia de datos.

16) ¿Qué pasaría si el agregador se lanza en paralelo a los bloques?
    Podría leer un archivo incompleto y calcular una media parcial; tendrías que reintentar, esperar o usar sincronización adicional.

17) ¿Cómo maneja el agregador contribuciones faltantes o count=0?
    Ignora bloques fuera de rango y acumula solo los que parsea correctamente; si total_count=0, define media 0.0 para no dividir por cero.

18) ¿Por qué el hijo P+2 calcula toda la media en paralelo?
    Sirve de referencia (baseline) para comparar precisión y tiempos frente a la agregación distribuida.

19) ¿Qué mide el padre al final exactamente?
    La diferencia absoluta entre la media agregada (P+1) y la total (P+2), y lista los tiempos individuales de los hijos 1..P+2.

20) ¿Cómo se registran los tiempos por hijo?
    Cada proceso mide con gettimeofday antes/después y escribe “tiempo=<ms>ms” en su propia línea del archivo.

21) ¿Por qué usas _exit(0) en los hijos?
    Para evitar vaciado doble de buffers stdio heredados tras fork, que podría duplicar salidas o desordenarlas.

22) ¿Qué pasa si un hijo de bloque falla antes de escribir?
    El agregador tendrá menos contribuciones y total_count menor; la media podría diferir. El padre debería detectar y reportar bloques ausentes.

23) ¿Cómo detectarías bloques ausentes?
    Contando líneas de tipo=bloque y comparando con P; si faltan, avisar o relanzar el cálculo de esos rangos.

24) ¿Qué importancia tiene truncar el archivo al inicio?
    Elimina residuos de ejecuciones previas para que el agregador y el padre no mezclen datos antiguos con los nuevos.

25) ¿Por qué guardas sum y count en lugar de la media del bloque?
    Para poder combinar sin sesgo: media_global = (Σ sum_i)/(Σ count_i). Promediar medias parciales pesa incorrectamente bloques con distinto tamaño.

26) ¿Qué ocurriría si promedias medias parciales?
    Introduces sesgo cuando los bloques tienen distinto count (por el resto); la media final sería incorrecta estadísticamente.

27) ¿Cómo escalarías a máquinas multinúcleo?
    Elige P≈núcleos lógicos, evita P>>cores para no sobresaturar; considera afinidad y tamaño de bloque para localidad de caché.

28) ¿Qué cambiaría si sustituyes el archivo por pipes o memoria compartida?
    Pipes simplifican el flujo y evitan parseo en disco pero requieren canalización activa; mmap/SHM elimina I/O pero exige sincronización de accesos.

29) ¿Cómo validarías que P+1 y P+2 dan resultados coherentes?
    Comprobar |media_agregado−media_total| < ε·max(1,|media_total|) con ε pequeño (p.ej., 1e-12) en tests automatizados.

30) ¿Qué harías para hacer el programa más robusto a señales o interrupciones?
    Instalar handlers mínimos (SIGINT/SIGTERM) para cerrar/ desbloquear el archivo limpiamente y registrar estado parcial.
