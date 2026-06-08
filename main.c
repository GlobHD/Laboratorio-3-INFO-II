#include <stdio.h>       //bibliotecas que recomendo gemini, algunas no se usan en este commit pero se dejan para futuros commits
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

int main()
{
  pid_t pid;
  int status, i;

  /* es para probar, pipe tonto */
  int pipe_dummy[2] = {-1, -1}; // arreglo de enteros que no se usa en este commit, pero se deja como parte de la estructura para javo

  printf("Padre: PID %d\n", getpid());//pido el id
  printf("[PADRE]: Creando procesos hijos...\n\n");

  /* i = 0 creará al hijo de Crédito.
   * i = 1 creará al hijo de Débito.
   */
  for(i = 0; i < 2; ++i)
  {
    pid = fork();// guardo el valo de la variable pid para cada proceso, el padre tendrá el PID del hijo y el hijo tendrá 0
    
    if(pid == -1)
    {
      perror("No se puede crear el proceso hijo");//mensaje de error
      exit(-1);
    }

    if(pid == 0)
    {
      // Si es el proceso hijo, ejecuta una tarea u otra 
      if(i == 0)
      {
        credito("credito.txt", pipe_dummy); // ejecuta la función de crédito, le paso el nombre del archivo de crédito y el pipe TONTO, luego se va a cambiar a un pipe real
      }
      else
      {
        debito("debito.txt", pipe_dummy); 
      }
      
      /* * El break rompe el bucle para que el hijo no siga ejecutando 
       * los ciclos del padre ni cree más procesos. Luego termina con exit() , abajo lo hace
       */
      break; 
    }
    else
    {
      printf("Nuevo proceso hijo creado: PID %d\n", pid);//esto seria para el padre, el hijo no entra acá porque tiene pid = 0, entonces el padre va a imprimir el PID de cada hijo que se crea
    }
  }

  // --- CÓDIGO DEL HIJO PARA SALIR --- 
  if(pid == 0)
  {
    // me aseguro de que lo hijos no continuen ejecutando codigo del padre
    exit(0); 
  }

  // --- CÓDIGO DEL PADRE --- 
  printf("\n[PADRE]: Esperando que los hijos procesen los archivos...\n\n");

  //Mientras se ejecutan los hijos el padre espera que terminen, con nwait espera a que cualquier hijo termine, devuelve el PID del hijo que terminó y el estado de salida del hijo, el padre hace esto dos veces porque tiene dos hijos, entonces espera a que ambos terminen
    for(i = 0; i < 2; ++i)//uno por cada hijo
  {
    pid_t pid_terminado = wait(&status);//
    
    if(pid_terminado > 0)
    {
      if(WIFEXITED(status) != 0)
      {
        printf("[PADRE]: Finalizó el proceso hijo con PID %d (Estado de salida: %d)\n", 
               pid_terminado, WEXITSTATUS(statusHijo));
      }
    }
  }

  printf("\n[PADRE]: Primer commit funcional completo. Todos los datos leídos.\n");

  return 0;
}

//----------------------------------------------
// Funciones que ejecuta cada proceso hijo
//----------------------------------------------

void credito(char *archivo_montos, int p[]) //funcion que ejecuta el hijo de crédito, recibe el nombre del archivo de montos y
// un arreglo de enteros (que no se usa en esta función, pero se deja como parte de la estructura para futuros commits)
{
  FILE *file = fopen(archivo_montos, "r"); 
  if(!file)
  {
    perror("Hijo Crédito: Error al abrir archivo");
    exit(-1);
  }

  printf("\tIniciando hijo CRÉDITO: PID %d leyendo '%s'\n", getpid(), archivo_montos);

  double monto;
  /* fscanf lee línea por línea el archivo de texto buscando números double */
  while(fscanf(file, "%lf", &monto) == 1)
  {
    printf("\t[PID %d - CRÉDITO]: Monto procesado localmente: +$%.2f\n", getpid(), monto);
    
    /* Pequeña espera en microsegundos para simular procesamiento concurrente */
    usleep(50000); 
  }

  fclose(file);
  printf("\tFinaliza hijo CRÉDITO: PID %d - Archivo completamente leído\n", getpid());
}

void debito(char *archivo_montos, int p[]) 
{
  FILE *file = fopen(archivo_montos, "r"); 
  if(!file)
  {
    perror("Hijo Débito: Error al abrir archivo");
    exit(-1);
  }

  printf("\tIniciando hijo DÉBITO: PID %d leyendo '%s'\n", getpid(), archivo_montos);

  double monto;
  while(fscanf(file, "%lf", &monto) == 1)
  {
    printf("\t[PID %d - DÉBITO]:  Monto procesado localmente: -$%.2f\n", getpid(), monto);
    usleep(70000); 
  }

  fclose(file);
  printf("\tFinaliza hijo DÉBITO: PID %d - Archivo completamente leído\n", getpid());
}