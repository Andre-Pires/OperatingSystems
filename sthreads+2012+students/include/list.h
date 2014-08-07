#include <cinema.h>


/*estrutura do elemento da lista */
typedef struct lst_iitem {
   Pedido * request;
   struct lst_iitem *next;
} lst_iitem_t;


/* list_t */
typedef struct {
   lst_iitem_t * first;
} list_t;


/*cria uma nova lista*/
list_t* lst_new();

/*destroi toda a lista*/
void lst_destroy(list_t *);

/*insere o elemento na lista*/
void lst_insert(list_t *list, Pedido * request);

/*procura o pedido na lista*/
Pedido * lst_search(list_t *list, int id);

/*remove o elemento da lista*/
void lst_remove(list_t *list, Pedido * request);

/*imprime os elementos da lista*/
void lst_print(list_t *list);
