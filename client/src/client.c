#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#define MAXDATASIZE 100
#define MAXDATASIZE_RESP 20000

char *read_command();

int main(int argc, char *argv[])
{

  if (argc != 3)
  {
    fprintf(stderr, "Uso del programa: %s <hostname> <puerto>\n", argv[0]);
    exit(1);
  }

  // Estos 2 son para la respuesta
  int numbytes;
  char buf[MAXDATASIZE_RESP];

  int sockfd;
  struct hostent *he;
  // informacion de la direccion de destino
  struct sockaddr_in conexion_servidor;

  // obtener informacion de host servidor
  if ((he = gethostbyname(argv[1])) == NULL)
  {
    perror("gethostbyname");
    exit(1);
  }

  // creamos el socket
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("socket");
    exit(1);
  }

  conexion_servidor.sin_family = AF_INET;
  conexion_servidor.sin_port = htons(atoi(argv[2]));
  conexion_servidor.sin_addr = *((struct in_addr *)he->h_addr);
  memset(&(conexion_servidor.sin_zero), '\0', 8); // poner a cero el resto de la estructura

  if (connect(sockfd, (struct sockaddr *)&conexion_servidor, sizeof(struct sockaddr)) == -1)
  {
    perror("connect");
    exit(1);
  }

  char *comando = NULL;

  do
  {
    // limpiamos el buffer
    memset(buf, 0, sizeof(buf));

    fputs("Servidor $ ", stdout);
    comando = read_command();

    size_t len_comando = strlen(comando) - 1;

    // Se envia el comando al servidor
    if (send(sockfd, comando, len_comando, 0) == -1)
    {
      perror("send()");
      exit(1);
    }

    if ((numbytes = recv(sockfd, buf, MAXDATASIZE_RESP - 1, 0)) == -1)
    {
      perror("recv");
      exit(1);
    }

    // Si el mensaje recibido es OK, el comando no tiene salida
    if (strcmp(buf, "OK") != 0)
    {
      printf("\n%s\n", buf);
    }
  } while (strcmp(comando, "exit\n") != 0);

  free(comando);
  close(sockfd);

  return 0;
}

char *read_command()
{
  char *input = NULL;
  size_t buffer_size = 0;

  if (getline(&input, &buffer_size, stdin) == -1)
  {
    perror("read command");
    exit(1);
  }

  return input;
}
