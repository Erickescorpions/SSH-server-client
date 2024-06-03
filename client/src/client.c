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

char* read_command();

int main(int argc, char *argv[]) {
  // Estos 2 son para la respuesta
  int numbytes;
  char buf[MAXDATASIZE_RESP];

  int sockfd;  
  struct hostent *he;
  struct sockaddr_in cliente; // informacion de la direccion de destino 

  if (argc != 3) {
    fprintf(stderr,"usage: client hostname puerto\n");
    exit(1);
  }

  if ((he=gethostbyname(argv[1])) == NULL) {  // obtener informacion de host servidor 
   perror("gethostbyname");
   exit(1);
  }

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
   perror("socket");
   exit(1);
  }

  cliente.sin_family = AF_INET; 
  cliente.sin_port = htons( atoi(argv[2]) ); 
  cliente.sin_addr = *((struct in_addr *)he->h_addr);
  memset(&(cliente.sin_zero), '\0',8);  // poner a cero el resto de la estructura 

  if (connect(sockfd, (struct sockaddr *)&cliente, sizeof(struct sockaddr)) == -1) {
    perror("connect");
    exit(1);
  }
  
  char* comando = NULL;

  do {
    fputs("Servidor $ ", stdout);
    comando = read_command();

    size_t len_comando = strlen(comando) - 1;

    // printf("Comando: %s\n", comando);
    // printf("El tamaño del comando es %ld\n", len_comando);

    // Se envia el comando al servidor
    if(send(sockfd, comando, len_comando, 0) == -1) {
      perror("send()");
      exit(1);
    }
    
    if ((numbytes = recv(sockfd, buf, MAXDATASIZE_RESP-1, 0)) == -1) {
      perror("recv");
      exit(1);
    }

    // Si el mensaje recibido es OK, el comando no tiene salida
    if(!(strcmp(buf, "OK") == 0)) {
      printf("\n%s\n",buf);
    }

    // Limpiamos el buffer
    memset(buf, 0, sizeof(buf));

  } while(strcmp(comando, "exit\n") != 0);

  free(comando);
  close(sockfd);

  return 0;
}

char* read_command() {
  char *input = NULL;
  size_t buffer_size = 0;

  if (getline(&input, &buffer_size, stdin) == -1){
    perror("read command");
    exit(1);
  }

  return input;
}
