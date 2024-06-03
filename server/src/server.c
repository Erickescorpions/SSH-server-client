#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h> 
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#include <stdbool.h>
#include "../include/LinkedList.h"

#define BUFFER_SIZE 20000

char **parse_command(char *string, int *command_list_size);
int execute(char **command_list, char *buf_respuesta, size_t buf_size);
void print_string_array(char **array, int size);

int main(int argc, char *argv[])
{

  if (argc == 1 || argc > 2)
  {
    fprintf(stderr, "Uso del programa: %s <puerto>\n", argv[0]);
    exit(1);
  }

  int numbytes;
  char buf_peticion[100];
  char buf_respuesta[BUFFER_SIZE];

  // variables para almacenar información del servidor y del cliente
  struct sockaddr_in servidor;
  struct sockaddr_in cliente; 

  int server_fd, cliente_fd;
  int sin_size_servidor;
  int sin_size_cliente;  

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("socket");
    exit(1);
  }

  // opciones del socket para reutilizar la dirección
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) == -1)
  {
    perror("Server-setsockopt() error!");
    exit(1);
  }

  printf("Direccion IP: %s\n", inet_ntoa(servidor.sin_addr));

  servidor.sin_family = AF_INET;            // Familia de direcciones
  servidor.sin_port = htons(atoi(argv[1])); // Puerto, convertido a orden de bytes de red
  servidor.sin_addr.s_addr = INADDR_ANY;    // Dirección IP del servidor
  memset(&(servidor.sin_zero), '\0', 8);    // Rellenar con ceros el resto de la estructura

  sin_size_servidor = sizeof(servidor);

  // asociamos el socket con la dirección y puerto especificados
  if (bind(server_fd, (struct sockaddr *)&servidor, sin_size_servidor) == -1)
  {
    perror("bind");
    exit(1);
  }

  // ponemos al servidor a escuchar peticiones
  if (listen(server_fd, 1) == -1)
  {
    perror("listen");
    exit(1);
  }

  sin_size_cliente = sizeof(cliente);

  do
  {
    puts("\nEsperando una nueva conexion...");

    // esperamos a una nueva conexion
    if ((cliente_fd = accept(server_fd, (struct sockaddr *)&cliente, &sin_size_cliente)) == -1)
    {
      perror("accept");
      exit(1);
    }
    printf("Nueva conexion cliente desde %s\n\n", inet_ntoa(cliente.sin_addr));

    FILE *fs;

    do
    {
      // limpiamos el buffer de la peticion
      memset(buf_peticion, 0, sizeof(buf_peticion));

      // limpiamos el buffer de respuesta
      memset(buf_respuesta, 0, sizeof(buf_respuesta));

      // leemos el mensaje del cliente
      if ((numbytes = recv(cliente_fd, buf_peticion, sizeof(buf_peticion) - 1, 0)) == -1)
      {
        perror("recv");
        exit(1);
      }

      printf("El comando recibido es: %s\n", buf_peticion);

      if (strcmp(buf_peticion, "exit") == 0)
      {
        // Mandamos mensaje de despedida al usuario
        char *mensaje_despedida = "Hasta luego 🤠";
        size_t tam_mensaje_despedida = strlen(mensaje_despedida);

        if (send(cliente_fd, mensaje_despedida, tam_mensaje_despedida, 0) < 0)
        {
          printf("ERROR: al enviar el mensaje de despedida al cliente\n");
          exit(1);
        }

        printf("Cerrando conexion con el cliente...\n");
        close(cliente_fd);

        break;
      }

      // parseamos la entrada
      int command_list_size = 0;
      // separamos el comando en un array de strings
      char **command_list = parse_command(buf_peticion, &command_list_size);
      // print_string_array(command_list, command_list_size);

      int num_bytes_leidos = execute(command_list, buf_respuesta, BUFFER_SIZE);

      printf("Comando ejecutado:\n%s", buf_respuesta);
      printf("Enviando respuesta el cliente...\n\n");
      // Aqui mandamos el mensaje de confirmacion cuando el comando no regresa nada por si solo (ej. mkdir)
      if (num_bytes_leidos == 0)
      {
        char *mensaje_exito = "OK";
        if (send(cliente_fd, mensaje_exito, strlen(mensaje_exito), 0) < 0)
        {
          printf("ERROR: al enviar el mensaje de éxito al cliente\n");
          exit(1);
        }
      }
      else if (send(cliente_fd, buf_respuesta, num_bytes_leidos, 0) < 0)
      {
        printf("ERROR: al enviar la salida del comando al cliente\n");
        exit(1);
      }
    } while (true);
  } while (true);

  // Cerrar las conexiones
  close(cliente_fd);
  close(server_fd);
  shutdown(server_fd, SHUT_RDWR);

  return 0;
}

char **parse_command(char *string, int *command_list_size)
{
  char *buffer;
  char *delimiter = " \t\r\n\a";

  buffer = strtok(string, delimiter);

  // comparamos el primer token por si se quiere salir del shell
  if (strcmp(buffer, "exit") == 0)
    return NULL;

  // creamos la lista enlazada
  LinkedList *ll = new_LinkedList();

  // separamos por palabras lo que se ingreso en el shell
  do
  {
    push_LinkedList(ll, buffer);
    buffer = strtok(NULL, delimiter);
  } while (buffer);

  // print_LinkedList(ll);

  // como execvp recibe una array de cadenas, pasamos la lista enlazada a un arreglo
  char **command_list = to_array_LinkedList(ll, command_list_size);

  // liberamos la memoria de la lista enlazada
  destroy_LinkedList(ll);

  return command_list;
}

int execute(char **command_list, char *buf_respuesta, size_t buf_size)
{
  int fd[2];
  int num_bytes_leidos = 0;

  // creamos la pipe
  if (pipe(fd) < 0)
  {
    // error al ejecutar el pipe
    perror("pipe");
    exit(1);
  }

  // creamos un proceso hijo
  int child_process = fork();

  switch (child_process)
  {
  case 0: // Proceso hijo
    // El proceso hijo va a ejecutar el comando
    // cerramos el fd de lectura del pipe
    close(fd[0]);
    // cambiamos la salida del comando
    dup2(fd[1], STDOUT_FILENO);
    // cerramos el pipe de escritura
    close(fd[1]);
    // ejecutamos el comando
    if (execvp(command_list[0], command_list) < 0) {
        perror("execvp");
        exit(1);
    }
    break;
  case -1: // Ocurrio un error
    perror("fork");
    exit(1);
    break;
  default: // Proceso padre
    // el proceso hijo va a esperar la respuesta
    // cerramos el fd de escritura
    close(fd[1]);
    // esperamos a que el proceso hijo termine
    waitpid(child_process, NULL, 0);
    // esperamos la respuesta
    num_bytes_leidos = read(fd[0], buf_respuesta, buf_size - 1);

    // cerramos el fd de lectura del pipe
    close(fd[0]);
    break;
  }

  return num_bytes_leidos;
}

void print_string_array(char **array, int size)
{
  printf("Tam del arreglo es %d\n", size);

  for (int i = 0; i < size; i++)
  {
    printf("El valor en el indice %d es: %s\n", i, array[i]);
  }
}