/*
 * queue.h -- definition and declarations of sthread queue
 */
#include <stdlib.h>
#include <stdio.h>


/* queue_element_t */
typedef struct gqueue_element {
    void *ele;
    struct gqueue_element *next;
    struct gqueue_element *prev;
} gqueue_element_t;

/* queue_t */
typedef struct {
  gqueue_element_t *first;
  gqueue_element_t *last;
} gqueue_t;

/* create_queue - allocates memory for queue_t and initiates the structure */
gqueue_t* create_gqueue();

/* delete_queue - frees all memory used by queue and its elements */
void delete_gqueue(gqueue_t *gqueue);

/* queue_is_empty - returns 1 if queue empty, else returns 0 */
int gqueue_is_empty(gqueue_t *queue);

/* queue_first - returns a pointer to the queue first element */
void* gqueue_first(gqueue_t *queue);

/* queue_insert - inserts a new element on the end of the queue */
void gqueue_insert(gqueue_t *queue, void *ele);

/* queue_remove - removes the first element of the queue */
void* gqueue_remove(gqueue_t *queue);

void * gqueue_find_remove(gqueue_t *queue, void *ele);

void * gqueue_find_next(gqueue_t *queue, void *ele);

