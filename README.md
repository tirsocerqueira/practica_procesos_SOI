# Práctica 1 Sistemas Operativos USC

## Notas Ejercicio 1

1) Compilado =>  gcc ejercicio1.c -o ejercicio1

2) Ejecución =>  Apertura de terminal y ejecución con ./ejercicio1
             =>  En la terminal comentar que pese a que el proceso hijo sea una copia del del padre, realmente solo copia virtualmente al proceso, mismas direcciones pero en espacios físicos diferentes
             =>  Comentar que ambos procesos compiten por el scanf (descriptor) y por eso da error
             =>  Entrar al fichero con cat compartido.txt y visualizar que padre escribe en un primer momento pero al compartir el descriptor ambos escriben y escriben variables con contenidos diferentes por ser espacios físicos distintos


## Notas Ejercicio 2

1) Compilado =>  gcc ejercicio2.c -o ejercicio2

2) Ejecución => Abrir dos terminales: una para ejecución de código y otra para comprobación de estados
             => Comando ejecución: ./ejercicio2
             => En la terminal donde no se ejecuta utilizar recurrentemente este comendo => ps -p AQUI_NUMERO_DE_PROCESO -o pid,ppid,state 
             => Irá mostrando los estados por los que pasan los procesos que se crearon => STAT Z (Zombie)
                                                                                        => STAT S (Sleep)
             => Hacer la ejecución con hijo Zombie => tendrá el estado Z+ al principio y tras unos segundos el comando ya no devolverá la información (muere) y en la terminal de ejecución se mostrará el estado 7 (número random para comprobar que es una salida peculiar)

             => Hacer una ejecución con Hijo Huérfano => tendrá estado S mientras el padre viva (segundos que espera al primer hijo zombie). Una vez recoge la salida del hijo muere. Si se lanza el comando anterior se mostrará el PPID 1 conforme init adopta el proceso 
                    => No se llega a mandar el printf después del comando exec ya que este carga toda la información (pila, hojas...) del nuevo proceso y eejecuta la función de línea de comandos, acabará el prompt listando los ficheros y seguramente se visualice la línea en negro
                                                                                
## Notas Ejercicio 3