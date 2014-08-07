/*
 * generic_queue.c - implementation of queue manipulation functions
 */
#include <generic_queue.h>

gqueue_element_t *empty_element_ptr;



gqueue_t* create_gqueue() {
  gqueue_t *queue;
  queue = (gqueue_t*) malloc(sizeof(gqueue_t));
  queue->first = NULL;
  queue->last = NULL;
  empty_element_ptr = NULL;
  return queue;
}

void delete_gqueue(gqueue_t *queue) {
  gqueue_element_t *pointer, *next;
  pointer = queue->first;
  while(pointer){
    next = pointer->next;
    free(pointer);
    pointer = next;
  }
  free(queue);
}

void gqueue_insert(gqueue_t *queue, void *ele) {

  gqueue_element_t *new_element;

  if(empty_element_ptr == NULL)
    new_element = (gqueue_element_t*) malloc(sizeof(gqueue_element_t));
  else {
    new_element = empty_element_ptr;
    empty_element_ptr = NULL;
  }

  new_element->ele = ele;
  new_element->next = NULL;
  new_element->prev = NULL;

  if(queue->first == NULL) {
    queue->first = new_element;
    queue->last = new_element;
    return;
  }
  queue->last->next = new_element;
  new_element->prev = queue->last;
  queue->last = new_element;
}


int gqueue_is_empty(gqueue_t *queue){
  if (queue->first == NULL)return 1;
  else return 0;
}


void *gqueue_first(gqueue_t *queue) {
  gqueue_element_t *pointer; 
  pointer = queue->first;
  return pointer==NULL?NULL:pointer->ele;
}

void * _gremove(gqueue_t *queue, gqueue_element_t *element){
  void *ele;

  if(element == NULL) return NULL;
  if(element->prev == NULL)
    queue->first = element->next;
  else
    element->prev->next = element->next;
  if(element->next == NULL)
    queue->last = element->prev;
  else
    element->next->prev = element->prev;
      
  ele = element->ele;
  element->ele = NULL;
  element->next = NULL;
  element->prev = NULL;
  
  if(empty_element_ptr)free(element);
  else empty_element_ptr = element;

 
  return ele;
}


void * gqueue_remove(gqueue_t *queue) {
  
  if(queue->first == NULL) return NULL;
  return _gremove(queue,queue->first);
}

void * gqueue_find_remove(gqueue_t *queue, void *ele) {
  gqueue_element_t *temp = queue->first;
  while(temp !=NULL && temp->ele != ele) {
    temp = temp->next;
  }
  if(temp!=NULL) return _gremove(queue,temp);
  return NULL;   

}

void * gqueue_find_next(gqueue_t *queue, void *ele) {
  gqueue_element_t *temp = queue->first;
  while(temp !=NULL && temp->ele != ele) {
    temp = temp->next;
  }
  if(temp!=NULL && temp->next != NULL) return temp->next->ele;
  return NULL;   
}


