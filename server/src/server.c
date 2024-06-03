#include <stdio.h>       // Librería estándar de E/S
#include <stdlib.h>      // Librería estándar de utilidades
#include <unistd.h>      // Definiciones de la API del sistema POSIX
#include <errno.h>       // Definiciones de códigos de error
#include <string.h>      // Funciones de manejo de cadenas
#include <sys/types.h>   // Definiciones de tipos de datos básicos del sistema
#include <sys/socket.h>  // Definiciones de la API de sockets
#include <netinet/in.h>  // Definiciones para sockets de Internet
#include <arpa/inet.h>   // Definiciones para operaciones de Internet
#include <sys/wait.h>    // Definiciones para manejar procesos hijos
#include <signal.h>      // Definiciones para señales

#include <stdbool.h>

#define LENGTH 20000     // Definición del tamaño del buffer
#define ARCHIVO_AUXILIAR "tmp.txt"

int main(int argc, char *argv[]) {

  if(argc == 1 || argc > 2) {
    fprintf(stderr, "Uso del programa: %s <puerto>\n", argv[0]);
    exit(1);
  }

  int numbytes;
  char buf_peticion[100];        // Buffer para recibir datos
  char buf_respuesta[LENGTH];    // Buffer para enviar datos

  // Estructuras para almacenar información del servidor y del cliente
  struct sockaddr_in servidor;   // Información sobre la dirección del servidor
  struct sockaddr_in cliente;    // Información sobre la dirección del cliente

  int server_fd, cliente_fd;     // Descriptores de archivo para el servidor y el cliente

  int sin_size_servidor;         // Tamaño de la estructura del servidor
  int sin_size_cliente;          // Tamaño de la estructura del cliente

  // Crear un socket
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("socket");
    exit(1);
  }

  // Configurar opciones del socket para reutilizar la dirección
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) == -1)
  {
    perror("Server-setsockopt() error!");
    exit(1);
  }
  
  printf("Direccion IP: %s\n", inet_ntoa(servidor.sin_addr));

  // Configurar la estructura del servidor
  servidor.sin_family = AF_INET;                // Familia de direcciones
  servidor.sin_port = htons(atoi(argv[1]));     // Puerto, convertido a orden de bytes de red
  servidor.sin_addr.s_addr = INADDR_ANY;        // Dirección IP del servidor
  memset(&(servidor.sin_zero), '\0', 8);        // Rellenar con ceros el resto de la estructura

  // Tamaño de la estructura del servidor
  sin_size_servidor = sizeof(servidor);

  // Asociar el socket con la dirección y puerto especificados
  if (bind(server_fd, (struct sockaddr *)&servidor, sin_size_servidor) == -1)
  {
    perror("bind");
    exit(1);
  }

  // Poner el socket en modo de escucha
  if (listen(server_fd, 1) == -1)
  {
    perror("listen");
    exit(1);
  }

  // Tamaño de la estructura del cliente
  sin_size_cliente = sizeof(cliente);

  FILE* fptr;
  // Creamos el archivo temporal donde vamos a guardar la salida del comando
  if((fptr = fopen(ARCHIVO_AUXILIAR, "w")) == NULL) {
    perror("Error al crear archivo temporal");
    exit(1);
  }
  // Inmediatamente lo cerramos
  fclose(fptr);

  do {

    puts("\nEsperando una nueva conexion...");
    // Aceptar una conexión entrante
    if ((cliente_fd = accept(server_fd, (struct sockaddr *)&cliente, &sin_size_cliente)) == -1)
    {
      perror("accept");
      exit(1);
    }
    printf("Nueva conexion cliente desde %s\n\n", inet_ntoa(cliente.sin_addr));

    FILE* fs;
    char *fs_name = ARCHIVO_AUXILIAR;

    do {

      // limpiamos el buffer de la peticion
      memset(buf_peticion, 0, sizeof(buf_peticion));

      // limpiamos el buffer de respuesta 
      memset(buf_respuesta, 0, sizeof(buf_respuesta));

      // limpiamos el archivo
      fs = fopen(fs_name, "w");
      fclose(fs);

      // Recibir datos del cliente
      if ((numbytes = recv(cliente_fd, buf_peticion, sizeof(buf_peticion) - 1, 0)) == -1)
      {
        perror("recv");
        exit(1);
      }

      printf("El comando recibido es: %s\n", buf_peticion);

      if(strcmp(buf_peticion, "exit") == 0) {
        // Mandamos mensaje de despedida al usuario
        char* mensaje_despedida = "Hasta luego 🤠";
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

      // Preparar el comando para redirigir su salida a un archivo
      // concadenamos en la posicion del numbytes leidos
      buf_peticion[numbytes] = ' ';
      buf_peticion[numbytes + 1] = '>';
      buf_peticion[numbytes + 2] = ' ';
      strcat(buf_peticion, ARCHIVO_AUXILIAR);

      // TODO: Cambiar la forma de ejecutar el comando
      // Ejecutar el comando recibido
      system(buf_peticion);
    
      // TODO: Cambiar la forma de leer la respuesta del comando
      fs = fopen(fs_name, "r");
      if (fs == NULL)
      {
        printf("ERROR: File %s not found on server.\n", fs_name);
        exit(1);
      }

      // Enviar el contenido del archivo al cliente
      
      bzero(buf_respuesta, LENGTH); 
      
      int fs_block_sz;
      int sin_respuesta = 1;
      while ((fs_block_sz = fread(buf_respuesta, sizeof(char), LENGTH, fs)) > 0)
      {
        sin_respuesta = 0;
        if (send(cliente_fd, buf_respuesta, fs_block_sz, 0) < 0)
        {
          printf("ERROR: al enviar la salida del comando al cliente\n");
          exit(1);
        }
        bzero(buf_respuesta, LENGTH);
      }
      fclose(fs);

      //Aqui mandamos el mensaje de confirmacion cuando el comando no regresa nada por si solo (ej. mkdir)
      if (sin_respuesta) {
        char* mensaje_exito = "OK";
        if (send(cliente_fd, mensaje_exito, strlen(mensaje_exito), 0) < 0) {
          printf("ERROR: al enviar el mensaje de éxito al cliente\n");
          exit(1);
        }
      }
      
      printf("Enviando respuesta el cliente\n");
    } while(true);
  } while(true);

  // Cerrar las conexiones
  close(cliente_fd);
  close(server_fd);
  shutdown(server_fd, SHUT_RDWR);

  return 0;
}
