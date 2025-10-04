## PREGUNTAS PRÁCTICA 2 SOI

### Ejercicio 1


### Ejercicio 2
1) ¿Qué diferencia hay entre un proceso zombie y un proceso huérfano?
    Un zombie es un proceso que ya ha terminado, pero cuyo padre no ha leído su estado de salida mediante wait() o waitpid().
    Un huérfano es un proceso vivo cuyo padre ha terminado antes que él; el sistema lo adopta, normalmente el proceso init o systemd.
2) ¿Por qué el proceso zombie sigue apareciendo en la tabla de procesos aunque haya terminado su ejecución?
    Porque el sistema debe conservar su información de salida (exit status) hasta que el padre la recoja.
    Esto evita que el padre pierda datos sobre cómo terminó su hijo.
3) ¿Quién elimina finalmente a un proceso zombie?
    Su padre, cuando llama a wait() o waitpid().
    Si el padre termina antes de hacerlo, init/systemd adopta al zombie y lo limpia automáticamente.
4) ¿Qué ocurre con el proceso huérfano cuando su padre termina? ¿Quién lo adopta?
    Lo adopta init (PID 1) o systemd, que pasa a ser su nuevo padre y se encargará de recoger su estado cuando termine.
5) ¿Qué diferencias hay entre el estado Z y S en la salida de ps?
    Z: proceso zombie, ya finalizado.
    S: proceso sleeping, temporalmente bloqueado esperando un evento (como sleep()).
6) ¿Qué sucede si el padre ejecuta wait() o waitpid() en este programa?
    El proceso zombie no se generaría, ya que el padre recogería la salida del primer hijo.
7) ¿Por qué es necesario hacer sleep() en el padre y en los hijos?
    Para controlar el orden de finalización y forzar las condiciones:
    El primer hijo muere antes que el padre → zombie.
    El segundo hijo vive más que el padre → huérfano.
8) ¿Qué pasaría si quitases el sleep(20) del padre?
    El padre podría terminar antes de que el primer hijo se vuelva zombie, y no verías el estado Z en la tabla de procesos.
9) ¿Qué pasaría si en lugar de exit(0) usases return 0; en los hijos?
    En este caso el efecto sería el mismo, porque main() termina y llama implícitamente a exit().
    Pero si el código estuviera en una función distinta, exit() garantiza la finalización inmediata del proceso, mientras que return solo sale de la función actual.
10) ¿Qué rol juega getpid() y getppid() y qué utilidad tienen en esta práctica?
    getpid() devuelve el PID del proceso actual.
    getppid() devuelve el PID del proceso padre.
    Sirven para verificar la relación padre-hijo y observar cómo cambia el PPID cuando el hijo queda huérfano.
11) ¿Por qué usas el tipo pid_t en lugar de int?
    Porque pid_t es el tipo estándar definido en POSIX para los identificadores de procesos, garantizando portabilidad entre sistemas UNIX/Linux.
12) ¿Qué devuelve fork() exactamente?
    En el padre, devuelve el PID del hijo (>0).
    En el hijo, devuelve 0.
    En caso de error, devuelve -1.
13) ¿Qué sucede si fork() falla? ¿Qué causas comunes pueden provocarlo?
    Si falla, no se crea el proceso hijo y se retorna -1.
    Las causas típicas son falta de memoria, límite de procesos por usuario o recursos del sistema agotados.
14) ¿Por qué el primer hijo termina con exit(0) inmediatamente?
    Para simular que muere antes que su padre y pueda volverse zombie mientras el padre sigue vivo.
15) ¿Por qué el segundo hijo duerme más que el padre?
    Para asegurar que el padre muera antes que él, y así el segundo hijo se quede huérfano.
16) ¿Cuál es el orden de ejecución esperado de los mensajes en pantalla? ¿Podría variar?
    Esperado:
    “Soy el padre…”
    “Soy el primer hijo…”
    “Soy el segundo hijo antes de que muera mi padre…”
    Pero puede variar, ya que los procesos se ejecutan de forma concurrente y el orden exacto depende del planificador del sistema operativo.
17) ¿Qué comando usarías para comprobar que realmente existe el proceso zombie?
    ps -p <pid> -o pid,ppid,state,cmd
    o simplemente:
    ps aux | grep defunct
18) ¿Qué comando usarías para verificar que el segundo hijo ha sido adoptado por init o systemd?
    ps -o pid,ppid,cmd | grep <pid_del_hijo>
    Verás que su PPID cambia a 1 (o al PID de systemd).
19) ¿Qué ocurriría si intercambiases el orden de los fork()?
    Se invertirían los roles: el primer hijo podría ser el huérfano y el segundo el zombie, dependiendo del orden y tiempos de sleep().
20) ¿Por qué en el código el padre no ejecuta ningún wait()?
    Porque precisamente se busca provocar el estado zombie del primer hijo.
    Si lo hiciera, el sistema eliminaría el proceso zombie inmediatamente.  

21) ¿Qué ocurriría si en lugar de sleep() usaras un bucle infinito en el padre?
    El padre no terminaría nunca, y el hijo zombie permanecería indefinidamente en la tabla de procesos.
22) ¿Por qué puede ser peligroso tener procesos zombies en un sistema real?
    Porque ocupan entradas en la tabla de procesos, un recurso limitado del sistema.
    Muchos zombies pueden impedir la creación de nuevos procesos.
23) ¿En qué situación práctica podrían aparecer procesos huérfanos?
    Cuando un servicio o programa padre finaliza antes que sus procesos hijos (por ejemplo, un daemon mal gestionado).
24) ¿Qué pasaría si el sistema tuviera demasiados procesos zombies?
    Se saturaría la tabla de procesos, y no se podrían crear nuevos procesos hasta que los zombies sean recogidos.
25) ¿Cómo podrías modificar este programa para evitar la creación del proceso zombie?
    Llamando a wait() o waitpid() después del primer fork() para limpiar al hijo.
    waitpid(pid1, NULL, 0);
26) ¿Podrías usar waitpid(pid1, NULL, WNOHANG) para limpiar al zombie sin bloquear el padre? ¿Cómo se vería eso en el código?
    Sí. Por ejemplo:
    if (waitpid(pid1, NULL, WNOHANG) > 0) {
        printf("Hijo recogido sin bloqueo.\n");
    }
    Esto limpia al zombie sin detener el flujo del programa.
27) ¿Qué diferencias hay entre usar /bin/ps y top para observar los procesos?
    ps: muestra una instantánea estática de los procesos.
    top: muestra información dinámica y en tiempo real, actualizándose continuamente.
28) ¿Qué resultado esperas ver al ejecutar /bin/ps -p <pid> -o pid,ppid,state?
    Para el primer hijo: estado Z o Z+.
    Para el segundo hijo: estado S o R (según si está durmiendo o ejecutando).
29) ¿Qué cambios verás en el PPID del segundo hijo antes y después de la muerte del padre?
    Antes: el PPID es el PID del padre original.
    Después: el PPID pasa a ser 1 (init/systemd).
30) ¿Cuánto tiempo permanece visible el proceso zombie antes de desaparecer?
    Durante el sleep(20) del padre, hasta que el padre termina.
    Cuando el padre muere, init adopta al zombie y lo elimina.
31) ¿Qué ocurre si reduces el sleep(20) del padre a sleep(5)?
    El padre podría morir antes de que se vea claramente el zombie, y el sistema lo limpiaría rápidamente.
    Sería más difícil observar el estado Z.
32) ¿Cómo comprobarías que el zombie realmente ha desaparecido cuando el padre muere?
    Repetir ps -p <pid> después de que el padre haya terminado.
    Si el zombie ya no aparece, significa que el sistema lo eliminó correctamente.   