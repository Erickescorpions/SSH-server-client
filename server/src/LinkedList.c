#include "../include/LinkedList.h"

Node *new_Node(char *value)
{
  Node *n = (Node *)malloc(sizeof(Node));
  n->value = value;
  n->next = NULL;

  return n;
}

LinkedList *new_LinkedList()
{
  LinkedList *l = (LinkedList *)malloc(sizeof(LinkedList));
  l->first = l->last = NULL;
  l->size = 0;

  return l;
}

void push_LinkedList(LinkedList *this, char *value)
{
  Node *n = new_Node(value);

  if (this->first == NULL)
  {
    this->first = this->last = n;
  }
  else
  {
    Node *aux = this->last;
    aux->next = n;
    this->last = n;
  }

  this->size++;
}

void print_LinkedList(LinkedList *this)
{
  Node *aux = this->first;

  for (size_t i = 0; i < this->size; i++)
  {
    printf("Valor guardado es: %s\n", aux->value);
    aux = aux->next;
  }
}

char **to_array_LinkedList(LinkedList *this, int *array_size)
{
  Node *aux = this->first;
  char **array = malloc(sizeof(char *) * (this->size + 1));

  for (size_t i = 0; i < this->size; i++)
  {
    int string_len = strlen(aux->value);
    array[i] = malloc(sizeof(char) * string_len);
    strcpy(array[i], aux->value);
    aux = aux->next;
  }

  array[this->size] = NULL;

  *array_size = this->size + 1;

  return array;
}

void destroy_LinkedList(LinkedList *this)
{
  if (this == NULL)
    return;

  Node *current = this->first;

  while (current != NULL)
  {
    Node *aux = current->next;
    free(current);
    current = aux;
  }

  free(this);
}