#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct Node {
    struct Node *next;
    char *value;
} Node;

typedef struct LinkedList {
    int size;
    Node* first;
    Node* last;
} LinkedList;

Node* new_Node(char* value);
LinkedList* new_LinkedList();
void destroy_LinkedList(LinkedList* this);

void push_LinkedList(LinkedList* this, char* value);
void print_LinkedList(LinkedList* this);
char** to_array_LinkedList(LinkedList* this, int* array_size);