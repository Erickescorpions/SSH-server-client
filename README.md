# SSH-server-client


## Instrucciones de compilacion

```
gcc server/src/server.c server/src/LinkedList.c -o server.out
./server.out <puerto>
```

```
gcc client/src/client.c -o client.out
./client <host:direccion ip> <puerto>
```