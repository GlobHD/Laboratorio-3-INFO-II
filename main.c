#include <stdio.h> //bibliotecas que recomendo gemini, algunas no se usan en este commit pero se dejan para futuros commits
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <errno.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

/*
 * Laboratorio III.1 - COMMIT 1: Estructura de procesos y lectura de archivos
 * Autores: Agustin Iñiguez, Javier Eberle
 * Repositorio GitHub: https://github.com/GlobHD/Laboratorio-3-INFO-II.git
 */

/* Prototipos del laboratorio */
void credito(char *archivo_montos, int p[]);
void debito(char *archivo_montos, int p[]);

// Estructura que residirá en la memoria compartida
typedef struct
{
  double saldo;
  sem_t semaforo;
} MemoriaCompartida;

// Puntero global a la memoria compartida
MemoriaCompartida *mem;

int main()
{
  // Variables para manejo de procesos e IPC
  pid_t pid;
  // El estado de salida de los hijos se almacena en esta variable para que el padre pueda verificar cómo terminaron los hijos.
  int status, i;

  // Pipes para comunicación entre procesos
  int pipe_credito[2];
  int pipe_debito[2];

  // Crear los pipes para crédito y débito. Cada pipe es un arreglo de dos enteros: el índice 0 es para lectura y el índice 1 es para escritura. Si la creación de alguno de los pipes falla, se imprime un mensaje de error y se termina el programa con un código de error.
  if (pipe(pipe_credito) == -1 || pipe(pipe_debito) == -1)
  {
    perror("Error al crear los pipes");
    exit(-1);
  }

  // Imprimir el PID del proceso padre para referencia. Esto es útil para depuración y para entender la estructura de procesos que se está creando.

  // El proceso padre es el que inicia la ejecución del programa y es responsable de crear los procesos hijos para crédito y débito. El PID del proceso padre se muestra al inicio para que sea fácil identificarlo en la salida de la consola, especialmente cuando se imprimen los PIDs de los procesos hijos.
  printf("Padre: PID %d\n", getpid());
  printf("[PADRE]: Creando procesos hijos...\n\n");

  // 1. Creamos la memoria compartid usando mmap
  mem = mmap(NULL, sizeof(MemoriaCompartida),
             PROT_READ | PROT_WRITE,
             MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  // Verificamos que la memoria se haya creado correctamente
  if (mem == MAP_FAILED)
  {
    perror("Error al crear la memoria compartida");
    exit(-1);
  }

  // 2. Inicializar las variables compartidas
  mem->saldo = 0.0;

  // 3. Inicializar el semáforo
  // El segundo parámetro '1' indica que se comparte entre procesos.
  // El tercer parámetro '1' es el valor inicial del semáforo (1 = disponible).
  if (sem_init(&mem->semaforo, 1, 1) == -1)
  {
    perror("Error al inicializar el semáforo");
    exit(-1);
  }

  /*
      i = 0 creará al hijo de Crédito.
      i = 1 creará al hijo de Débito.


  // El bucle se ejecuta dos veces para crear dos procesos hijos, uno para crédito y otro para débito. En cada iteración, se llama a fork() para crear un nuevo proceso. El proceso padre recibirá el PID del hijo creado, mientras que el proceso hijo recibirá 0. Dependiendo del valor de i, el proceso hijo ejecutará la función correspondiente (credito o debito) y luego romperá el bucle para evitar que siga creando más procesos. El proceso padre, por su parte, imprimirá el PID de cada hijo creado.

  */

  // Bucle de creación de procesos
  for (i = 0; i < 2; ++i)
  {
    // Crear un nuevo proceso hijo
    pid = fork();

    // Verificar si la creación del proceso hijo fue exitosa.
    // Si fork() devuelve -1, significa que hubo un error al crear el proceso hijo, por lo que se imprime un mensaje de error y se termina el programa con un código de error.
    if (pid == -1)
    {
      perror("No se puede crear el proceso hijo");
      exit(-1);
    }

    // Si pid es 0, estamos en el proceso hijo. Dependiendo del valor de i, el proceso hijo ejecutará la función correspondiente (credito o debito) y luego romperá el bucle para evitar que siga creando más procesos. El proceso padre, por su parte, imprimirá el PID de cada hijo creado.
    if (pid == 0)
    {

      // --- CÓDIGO DEL PROCESO HIJO ---
      if (i == 0)
      {
        // Hijo 0: Crédito
        close(pipe_credito[0]); // El hijo no lee, cierra el extremo de lectura
        close(pipe_debito[0]);  // Cierra el pipe del otro proceso por seguridad
        close(pipe_debito[1]);

        credito("credito.txt", pipe_credito);
      }
      else
      {
        // Hijo 1: Débito
        close(pipe_debito[0]);  // El hijo no lee, cierra el extremo de lectura
        close(pipe_credito[0]); // Cierra el pipe del otro proceso por seguridad
        close(pipe_credito[1]);

        debito("debito.txt", pipe_debito);
      }

      // El break rompe el bucle para que el hijo no siga creando procesos
      break;
    }
    else
    {
      // --- CÓDIGO DEL PROCESO PADRE ---
      printf("Nuevo proceso hijo creado: PID %d\n", pid);

      // El padre no va a escribir en los pipes, solo va a leer.
      // Es vital que el padre cierre sus extremos de escritura para que
      // más adelante pueda detectar cuándo los hijos cierran el pipe.
      if (i == 0)
      {
        close(pipe_credito[1]);
      }
      else
      {
        close(pipe_debito[1]);
      }
    }
  }

  // --- CÓDIGO DEL HIJO PARA SALIR ---
  if (pid == 0)
  {
    // me aseguro de que lo hijos no continuen ejecutando codigo del padre
    exit(0);
  }

  // --- CÓDIGO DEL PADRE ---
  printf("\n[PADRE]: Monitoreando las transacciones de los hijos...\n\n");

  double monto_leido;
  int bytes_credito = 1, bytes_debito = 1;
  int hijos_activos = 2;

  // Utilizamos un enfoque de lectura no bloqueante o alternada para leer de ambos pipes
  // Para simplificar, leeremos alternadamente usando read()
  // Importante: read() es bloqueante por defecto. Si un hijo tarda mucho y el otro envía rápido,
  // el padre podría quedarse esperando al hijo lento.
  // Para este laboratorio (donde los usleep son cortos y similares), una lectura secuencial o
  // configurar los pipes como no bloqueantes (O_NONBLOCK) es necesario.

  // Configuramos los extremos de lectura de los pipes como NO BLOQUEANTES
  /*
    Ref propia:
    fcntl(..., O_NONBLOCK): Configura los extremos de lectura de ambos pipes para que la función read() no se quede bloqueada esperando datos. Si el pipe está vacío, read() devolverá -1 inmediatamente, lo cual nos permite pasar a revisar el otro pipe.

  */

  fcntl(pipe_credito[0], F_SETFL, O_NONBLOCK);
  fcntl(pipe_debito[0], F_SETFL, O_NONBLOCK);

  int credito_abierto = 1;
  int debito_abierto = 1;
  int leidos; // Variable temporal para guardar lo que devuelve read()

  while (credito_abierto || debito_abierto)
  {
    // Intentar leer de Crédito si sigue abierto
    if (credito_abierto)
    {
      leidos = read(pipe_credito[0], &monto_leido, sizeof(double));
      if (leidos > 0)
      {
        printf("[PADRE] Recibió de CRÉDITO: +$%.2f\n", monto_leido);
      }
      else if (leidos == 0)
      {
        printf("[PADRE] El pipe de CRÉDITO se ha cerrado.\n");
        credito_abierto = 0; // Marcamos como cerrado para no volver a entrar
      }
    }

    // Intentar leer de Débito si sigue abierto
    if (debito_abierto)
    {
      leidos = read(pipe_debito[0], &monto_leido, sizeof(double));
      if (leidos > 0)
      {
        printf("[PADRE] Recibió de DÉBITO: -$%.2f\n", monto_leido);
      }
      else if (leidos == 0)
      {
        printf("[PADRE] El pipe de DÉBITO se ha cerrado.\n");
        debito_abierto = 0; // Marcamos como cerrado para no volver a entrar
      }
    }

    // Pausa corta para no saturar el procesador
    usleep(10000);
  }

  // Esperamos a que los procesos hijos terminen formalmente
  for (i = 0; i < 2; ++i)
  {
    wait(NULL);
  }

  printf("\n[PADRE]: Ambos hijos han finalizado. Saldo final en memoria compartida: $%.2f\n", mem->saldo);

  // Limpieza de recursos (Opcional pero recomendado)
  sem_destroy(&mem->semaforo);
  munmap(mem, sizeof(MemoriaCompartida));

  return 0;
}
//----------------------------------------------
// Funciones que ejecuta cada proceso hijo
//----------------------------------------------

// Funcion que  ejecuta el hijo de crédito, recibe el nombre del archivo de montos y un arreglo de enteros que representa el pipe para comunicarse con el padre.
void credito(char *archivo_montos, int p[])
{
  FILE *file = fopen(archivo_montos, "r");
  if (!file)
  {
    perror("Hijo Crédito: Error al abrir archivo");
    exit(-1);
  }

  printf("\tIniciando hijo CRÉDITO: PID %d leyendo '%s'\n", getpid(), archivo_montos);

  double monto;
  while (fscanf(file, "%lf", &monto) == 1)
  {
    // Bloquear semáforo
    sem_wait(&mem->semaforo);

    // Sumar al saldo
    mem->saldo += monto;

    // Liberar semáforo
    sem_post(&mem->semaforo);

    // Enviar monto al padre por el pipe (usamos p[1] que es el extremo de escritura)
    write(p[1], &monto, sizeof(double));

    // Usamos fflush(stdout) para asegurar que la impresión no se pise en la consola
    // Referencia propia:
    /*
    - Su sintaxis es int fflush(FILE *stream) retornando 0 si tiene éxito o EOF si ocurre un error.
    - fflush(stdout): Fuerza a que el sistema imprima inmediatamente en la consola cualquier texto      pendiente que esté retenido en el búfer. Es útil cuando usas printf sin un salto de línea (\n) y necesitas que el mensaje aparezca en pantalla al instante.
    fflush(archivo): Vuelca al disco duro los datos pendientes de un archivo que fue abierto previamente con fopen y está siendo escrito.
    */

    printf("\t[PID %d - CRÉDITO]: Acreditado: +$%.2f | Saldo parcial: $%.2f\n", getpid(), monto, mem->saldo);

    fflush(stdout);

    usleep(50000);
  }

  // 5. Cerrar el pipe de escritura y el archivo
  close(p[1]);
  fclose(file);
  printf("\tFinaliza hijo CRÉDITO: PID %d\n", getpid());
}

void debito(char *archivo_montos, int p[])
{
  FILE *file = fopen(archivo_montos, "r");
  if (!file)
  {
    perror("Hijo Débito: Error al abrir archivo");
    exit(-1);
  }

  printf("\tIniciando hijo DÉBITO: PID %d leyendo '%s'\n", getpid(), archivo_montos);

  double monto;
  while (fscanf(file, "%lf", &monto) == 1)
  {
    // 1. Bloquear semáforo
    sem_wait(&mem->semaforo);

    // 2. Sección Crítica: Restar al saldo
    mem->saldo -= monto;

    // 3. Liberar semáforo
    sem_post(&mem->semaforo);

    // 4. Enviar monto al padre por el pipe
    write(p[1], &monto, sizeof(double));

    // CORRECCIÓN DE LOS TEXTOS AQUÍ:
    printf("\t[PID %d - DÉBITO]: Debitado: -$%.2f | Saldo parcial: $%.2f\n", getpid(), monto, mem->saldo);
    fflush(stdout);

    usleep(50000);
  }

  // 5. Cerrar el pipe de escritura y el archivo
  close(p[1]);
  fclose(file);
  printf("\tFinaliza hijo DEBITO: PID %d\n", getpid());
}