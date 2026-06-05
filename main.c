#include <stdio.h>       
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
 * Repositorio GitHub: 
 */

/* Prototipos obligatorios de la guía de laboratorio */
void credito(char *archivo_montos, int p[]);
void debito(char *archivo_montos, int p[]);

int main()
{
  pid_t pid;
  int status, i;

  /* Temporal: pasamos un array dummy ya que no usamos pipes en este commit */
  int pipe_dummy[2] = {-1, -1}; 

  printf("Padre: PID %d\n", getpid());
  printf("[PADRE]: Creando procesos hijos...\n\n");

  /* * Estructura de la cátedra: Un solo bucle para crear ambos hijos.
   * i = 0 creará al hijo de Crédito.
   * i = 1 creará al hijo de Débito.
   */
  for(i = 0; i < 2; ++i)
  {
    pid = fork();
    
    if(pid == -1)
    {
      perror("No se puede crear el proceso hijo");
      exit(-1);
    }

    if(pid == 0)
    {
      /* Si es el proceso hijo, según el índice ejecuta una tarea u otra */
      if(i == 0)
      {
        credito("credito.txt", pipe_dummy); [cite: 10, 31]
      }
      else
      {
        debito("debito.txt", pipe_dummy); [cite: 10, 30]
      }
      
      /* * El break rompe el bucle 'for' para que el hijo no siga ejecutando 
       * los ciclos del padre ni cree más procesos. Luego termina con exit().
       */
      break; 
    }
    else
    {
      printf("Nuevo proceso hijo creado: PID %d\n", pid);
    }
  }

  /* --- CÓDIGO DEL HIJO AL ROMPER EL BUCLE --- */
  if(pid == 0)
  {
    /* Los hijos finalizan de manera limpia con exit() según la guía */ [cite: 38]
    exit(0); 
  }

  /* --- CÓDIGO EXCLUSIVO DEL PADRE --- */
  printf("\n[PADRE]: Esperando que los hijos procesen los archivos...\n\n");

  /* Otro bucle dedicado únicamente a esperar que terminen los 2 hijos */
  for(i = 0; i < 2; ++i)
  {
    pid_t pid_terminado = wait(&status);
    
    if(pid_terminado > 0)
    {
      if(WIFEXITED(status) != 0)
      {
        printf("[PADRE]: Finalizó el proceso hijo con PID %d (Estado de salida: %d)\n", 
               pid_terminado, WEXITSTATUS(status));
      }
    }
  }

  printf("\n[PADRE]: Primer commit funcional completo. Todos los datos leídos.\n");

  return 0;
}

//----------------------------------------------
// Funciones que ejecuta cada proceso hijo
//----------------------------------------------

void credito(char *archivo_montos, int p[]) [cite: 31]
{
  FILE *file = fopen(archivo_montos, "r"); [cite: 33]
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

void debito(char *archivo_montos, int p[]) [cite: 30]
{
  FILE *file = fopen(archivo_montos, "r"); [cite: 33]
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