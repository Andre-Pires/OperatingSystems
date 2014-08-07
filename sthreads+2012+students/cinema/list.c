#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cinema.h>
#include <buffer.h>
#include <cinema_map.h>
#include <list.h>


/*cria uma nova lista*/
list_t* lst_new(){
	list_t *list;
	list = (list_t*) malloc(sizeof(list_t));
	list->first = NULL;
	return list;
}


/*destroi toda a lista*/
void lst_destroy(list_t *list){

	lst_iitem_t * aux;
	aux=list->first;

	while(aux){
		aux=aux->next;
		 free(aux);
	}
	free(list);
}


/*insere o elemento na lista*/
void lst_insert(list_t *list, Pedido * request){
	
	lst_iitem_t* aux = (lst_iitem_t*)malloc(sizeof(lst_iitem_t)); 
	lst_iitem_t* aux2 = list->first;
	aux->request = request;
	aux->next = NULL;
	
	if( list->first == NULL ) 
		list->first = aux;
	else {
		while ( aux2->next != NULL ) {
			aux2 = aux2->next;
		}
		aux2->next = aux;
	}
}


/*remove o elemento da lista*/
void lst_remove(list_t *list, Pedido *p){
	
	int c=0;
	
	lst_iitem_t *aux=list->first;
	lst_iitem_t *rem;
	
	if(aux == NULL) 
		return; 
	if(aux->request==p) {
		list->first = aux->next;
		return;
	}
	while (aux->next != NULL){
		
		if (aux->next->request == p){
			rem = aux->next;
			aux->next = aux->next->next;
			free(rem);
			break;
		}
		aux = aux->next;
		c++;
	}
}


/*procura o pedido na lista*/
Pedido * lst_search(list_t *list, int id){
	
	lst_iitem_t * aux, *ant;
  	aux=list->first;
  
	if (aux== NULL) 
		return NULL;

	if(aux->request->id == id) 
	 	return aux->request;

	ant=aux;
	aux=aux->next;
	
	while(aux){
		if(aux->request->id == id)
			return aux->request;
		else{
			aux=aux->next;
			ant=ant->next;
		}
	}
	return NULL;
}


/*imprime os elementos da lista*/
void lst_print(list_t *list){

	lst_iitem_t* aux = list->first;
	int i;
	
	if (list->first == NULL){
		printf("Empty list.\n");
		return;
	}
	while (aux != NULL){
		if(aux->request->tipo == 2){
			printf("{ Preres:%d #seats:%d - ", aux->request->id, aux->request->dimensao);
			for (i = 0; i < aux->request->dimensao; ++i)
				printf("[%d,%d]", aux->request->lugares[i].row, aux->request->lugares[i].column);
			printf(" }\n");
		}
		if(aux->request->tipo == 1){
			printf("{ Res:%d #seats:%d - ", aux->request->id, aux->request->dimensao);
			for (i = 0; i < aux->request->dimensao; ++i)
				printf("[%d,%d]", aux->request->lugares[i].row, aux->request->lugares[i].column);
			printf(" }\n");
		}
		aux = aux->next;
	}
}
