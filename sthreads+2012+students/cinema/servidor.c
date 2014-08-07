#include <stdio.h>
#include <stdlib.h>
#include <sthread.h>
#include <semaphore.h>
#include <buffer.h>
#include <cinema.h>
#include <cinema_map.h>
#include <list.h>
#include <time.h>


/*constantes*/
#define PADDING 4


/*inicializa as variáveis globais*/
int NUM_ROWS;
int NUM_COLUMNS;
int NUM_RES;
int NUM_PRERES;
int NUM_EXP;
sthread_mutex_t** matrix = NULL;
sthread_mutex_t Mutex1;
sthread_mutex_t Mutex2;
list_t * reservas;
list_t * prereservas;


/*inicializa matriz auxiliar (baseada na inicialização do mapa)*/
void init_matrix(num_rows, num_cols){

	NUM_COLUMNS = num_cols;
	NUM_ROWS = num_rows;
	NUM_RES = 0;
	NUM_PRERES = 0;
	NUM_EXP = 0;

	int i,j;
	matrix = calloc(num_rows,sizeof(sthread_mutex_t*));
	for(i = 0;i<num_rows;i++) 
		matrix[i]=calloc(num_cols,sizeof(sthread_mutex_t));

	for(i = 0;i<num_rows;i++){
		for (j=0; j<num_cols; j++) {
			matrix[i][j]= sthread_mutex_init();
		}
	}
}


/*compara os lugares (função para qsort)*/
int compara(const void * a, const void * b){
	
	Coords_t lugar_a = *((Coords_t*)a);				/*inicializa variáveis com os lugares recebidos*/
	Coords_t lugar_b = *((Coords_t*)b);
	
	if(lugar_a.row == lugar_b.row && lugar_a.column == lugar_b.column)
		return 0;									/*se os lugares forem iguais a ordem mantem-se*/
	else{
		if(lugar_a.row == lugar_b.row){
			if(lugar_a.column < lugar_b.column)
				return -1;							/*se a linha for a mesma e a coluna for menor, troca-os*/
			else return 1;							/*se n, mantem a ordem*/
		}
		else{
			if(lugar_a.row < lugar_b.row)
				return -1;							/*se a linha for menor, troca-os*/
			else return 1;							/*se n, a ordem mantém-se*/
		}
	}
}


/*ordena os lugares do pedido*/
int ordena(Pedido * request){

	int i;
	int res = 0;
	int dimensao = request->dimensao;

	qsort(request->lugares, request->dimensao , sizeof(Coords_t *), compara);
	/*dependendo do valor que recebe da função compara, o qsort ordena o vetor de lugares*/

	/*ciclo que impede que sejam aceites pedidos com lugares fora do mapa do cinema*/
	for (i = 0; i < dimensao; ++i){
		if (request->lugares[i].row > NUM_ROWS-1 || request->lugares[i].column > NUM_COLUMNS-1){
			res = -1;
			break;
		}
	}

	return res;
}


/*tranca (individualmente) os lugares que recebe, na matriz auxiliar*/
void lock(Coords_t * lugares, int dimensao){
	int i;

	for (i = 0; i < dimensao; ++i){
			sthread_mutex_lock(matrix[lugares[i].row][lugares[i].column]);
	}
}



/*destranca (individualmente) os lugares que recebe, na matriz auxiliar*/
void unlock(Coords_t * lugares, int dimensao){
	int i;

	for (i = dimensao-1; i >= 0; --i){
			sthread_mutex_unlock(matrix[lugares[i].row][lugares[i].column]);	
	}
}



/*marca os lugares de um determinado pedido, no mapa do cinema*/
void marca_lugares(Pedido * request){

	int i;											
	int dim = (request->dimensao);
	status_t * status= NULL;
	Coords_t* seats = request->lugares;

	for (i = 0; i < dim; ++i){

		status = backend_get_state(seats[i].row, seats[i].column);
		/*verifica se cada lugar que se pretende reservar está livre*/

			if (status->state != 0){
				request->resultado = -1;			/*se um dos lugares estiver ocupado, falha e retorna -1 como resultado*/
				break;
			}
	}

	
	if (request->resultado == 0){					/*se puder realizar a reserva/prereserva*/
		if(request->tipo == 1){						/*se se tratar de uma reserva*/
			for (i = 0; i < dim; ++i)
				backend_reserva(seats[i].row, seats[i].column, request->id);
				/*reserva os lugares no mapa do cinema*/
				
			sthread_mutex_lock(Mutex1);				/*tranca o mutex*/

			lst_insert(reservas, request);			/*insere o pedido na lista de reservas*/
			NUM_RES++;								/*aumenta o numero de reservas*/
			
			sthread_mutex_unlock(Mutex1);			/*destranca o mutex*/
		}
		else if(request->tipo == 2){				/*se se tratar de uma prereserva*/
			for (i = 0; i < dim; ++i)
				backend_prereserva(seats[i].row, seats[i].column, request->tempo_vida, request->id);
				/*prereserva os lugares no mapa do cinema*/
				
			sthread_mutex_lock(Mutex2);				/*tranca o mutex*/

			request->tempo_entrada = time(NULL);	/*guarda o tempo de entrada do pedido*/
			lst_insert(prereservas, request);		/*insere o pedido na lista de prereservas*/
			NUM_PRERES++;							/*aumenta o numero de prereservas*/

			sthread_mutex_unlock(Mutex2);			/*destranca o mutex*/
		}
	}
}


/*cancela os lugares do pedido*/
int cancela_lugares(Pedido * request, Pedido * backup){

	int i;											/*inicializa as variáveis*/
	int dim = (backup->dimensao);
	Coords_t* seats = backup->lugares;
		
	for (i = 0; i < dim; ++i)
		request->resultado = backend_cancela(seats[i].row, seats[i].column);
		/*coloca no resultado do pedido o retorno da tentativa de cancelamento no mapa*/

	if(request->resultado != -1){					/*se for bem sucedido*/
		if (backup->tipo == 1){						/*se se tratar de uma reserva*/
			
			sthread_mutex_lock(Mutex1);				/*tranca o mutex*/

			lst_remove(reservas, backup); 			/*retira o pedido da lista de reservas*/
			NUM_RES--;								/*decrementa o número de reservas*/

			sthread_mutex_unlock(Mutex1);			/*destranca o mutex*/
			return 0;								/*bem sucedido*/
		
		}
		else if(backup->tipo == 2){					/*se se tratar de uma prereserva*/

			sthread_mutex_lock(Mutex2);				/*tranca o mutex*/

			lst_remove(prereservas, backup); 		/*retira o pedido da lista de prereservas*/
			NUM_PRERES--;							/*decrementa o numero de prereservas*/

			sthread_mutex_unlock(Mutex2);			/*destranca o mutex*/
			return 0;								/*bem sucedido*/
		}
	}
	return -1;										/*se falhar*/
}


/*reserva o pedido recebido*/
int reserva_pedido(Pedido * request){

	int res = 0;

	res = ordena(request);								/*ordena o vetor de lugares*/
	if (res == 0) {
		lock(request->lugares, request->dimensao);		/*tranca os lugares pretendidos*/
		marca_lugares(request);							/*marca lugares no mapa*/
		unlock(request->lugares, request->dimensao);	/*destranca os lugares pretendidos*/
	
	} else request->resultado = -1;

	return 0;											/*bem sucedido*/
}


/*cancela o pedido recebido*/
int cancelar_pedido(Pedido * request){

	Pedido * backup;								/*cria um pedido de backup*/

	sthread_mutex_lock(Mutex1);						/*tranca o mutex*/
	sthread_mutex_lock(Mutex2);						/*tranca o mutex*/

	backup = lst_search(reservas, request->id);		/*procura na lista de reservas o pedido com o id de 'request'*/
	if (backup == NULL)		
		backup = lst_search(prereservas, request->id);
		/*se n encontrar, procura na lista de prereservas*/
	
	sthread_mutex_unlock(Mutex2);					/*destranca o mutex*/
	sthread_mutex_unlock(Mutex1);					/*destranca o mutex*/

	if(backup != NULL){								/*se tiver recebido um backup válido*/
		lock(backup->lugares, backup->dimensao);	/*tranca os lugares pretendidos*/
		cancela_lugares(request, backup);			/*cancela os lugares pretendidos*/
		unlock(backup->lugares, backup->dimensao);	/*destranca os lugares pretendidos*/
		return 0;									/*bem sucedido*/
	}
	else return -1;									/*se falhar*/
}


/*atualiza a lista de prereservas*/
void actualiza_pre(list_t * prereservas){

	int i;											
	int dim;
	Coords_t* seats;
	time_t tempo;
	lst_iitem_t * aux;

	sthread_mutex_lock(Mutex2);						/*tranca o mutex*/

	aux = prereservas->first;						/*aux é a primeira posiçao da lista*/
	while(aux != NULL){								/*enquanto não chega à ultima posição da lista*/
		dim =  aux->request->dimensao;				/*copia a dimensão do pedido para 'dim'*/
		seats = aux->request->lugares;				/*copia os lugares do pedido para 'seats'*/
		tempo = time(NULL);							/*regista o tempo no momento*/

		if(difftime (tempo, aux->request->tempo_entrada) >= aux->request->tempo_vida){
			lock(seats, dim);						/*se já tiver expirado o tempo de vida da prereserva, tranca os lugares*/

			for (i = 0; i < dim; ++i)
				backend_cancela(seats[i].row, seats[i].column);
				/*cancela os lugares pretendidos no mapa do cinema*/

			NUM_PRERES--;							/*decrementa o número de prereservas*/
			NUM_EXP++;								/*incrementa o número de prereservas expiradas*/

			unlock(seats, dim);						/*destranca os lugares*/
			lst_remove(prereservas, aux->request);	/*remove da lista de prereservas*/
		}
		aux = aux->next;							/*avança para a proxima posição da lista*/
	}

	sthread_mutex_unlock(Mutex2);					/*destranca o mutex*/
}


/*confirma uma prereserva*/
int conf_prereserva(Pedido * request){

	int i;											
	Pedido * backup;
	int dim;
	Coords_t* seats;

	backup = lst_search(prereservas, request->id);	/*procura na lista de prereservas o pedido com o id do pedido recebido*/

	if (backup != NULL)	{							/*se encontrar o pedido*/
	
		sthread_mutex_lock(Mutex2);					/*tranca o mutex*/
		sthread_mutex_lock(Mutex1);					/*tranca o mutex*/
	
		lst_remove(prereservas, backup);			/*remove da lista de prereservas*/
		NUM_PRERES--;								/*diminui o numero de prereservas*/
		backup->tipo = 1;							/*altera o tipo do pedido*/
		backup->tempo_vida= -1;						/*invalida o tempo de vida da prereserva*/
		
		dim = backup->dimensao;						/*copia a dimensão do vetor de lugares do pedido para 'dim'*/
		seats = backup->lugares;					/*copia os lugares do pedido para 'seats'*/

		lst_insert(reservas, backup);				/*insere o pedido na lista de reservas*/
		NUM_RES++;									/*incrementa o numero de reservas*/

		sthread_mutex_unlock(Mutex1);				/*destranca o mutex*/
		sthread_mutex_unlock(Mutex2);				/*destranca o mutex*/

		lock(backup->lugares, backup->dimensao);	/*tranca os lugares do pedido*/

		for (i = 0; i < dim; ++i){
			backend_cancela(seats[i].row, seats[i].column);
			/*cancela os lugares pretendidos no mapa do cinema*/
			backend_reserva(seats[i].row, seats[i].column, backup->id);
			/*reserva os lugares pretendidos no mapa do cinema*/
		}

		unlock(backup->lugares, backup->dimensao);	/*tranca os lugares do pedido*/
		request->resultado = 0;						/*define o resultado do pedido como bem sucedido*/
	}
	else request->resultado = -1;					/*define o resultado do pedido como falha*/
	return 0;										/*bem sucedido*/
}


/*mostra o mapa do cinema, de acordo com o indicado em 'show.c'*/
void show_cinema(){

	int i,j;										


  	for (j=0; j<(NUM_COLUMNS)*(PADDING+1); j++) 
		printf("-");

	printf("\n");

	for(i = 0;i<(NUM_ROWS);i++) {
		printf("|");
		for (j=0; j<(NUM_COLUMNS); j++) {
			status_t* s=backend_get_state(i,j);
			char c='E'; //'E' is used to encode any unexpected status returned by the back_end (note this should never happen!)
			switch (s->state) {
				case 0: c='F'; break;
				case 1: c='P'; break;
				case 2: c='R'; break;
			}
			printf("%c|",c);
   		}
		printf("\n");
	}

	for (j=0; j<(NUM_COLUMNS)*(PADDING+1); j++) 
		printf("-");

	printf("\n");


	//print map state - including res_id in case seat is (pre-)reserved

	for(i = 0;i<(NUM_ROWS);i++) {
		printf("<");
		for (j=0; j<(NUM_COLUMNS); j++) {
		status_t* s=backend_get_state(i,j);
		char c='E'; //'E' is used to encode any unexpected status returned by the back_end (note this should never happen!)
		switch (s->state) {
			case 0: c='F';break;
			case 1: c='P';break;
			case 2: c='R';break;
		}
			printf("%c:%3d|",c,s->id);
   		}
	printf("\n");
	}


	printf("#res:%d\n",NUM_RES);
	printf("#preres:%d\n",NUM_PRERES);
	printf("#expired:%d\n",NUM_EXP);
	
	printf("-Reservations:\n");
	lst_print(reservas);
	printf("-Prereservations:\n");
	lst_print(prereservas);
}


/*trata o pedido do buffer*/
void * trata_pedido(void * arg){

	Pedido * request;								/*cria pedido 'request'*/

	while(1){

		request = le_pedido();						/*vai buscar o pedido ao buffer*/

		if (request->tipo == 1 || request->tipo == 2){
			actualiza_pre(prereservas); 			/*atualiza as prereservas*/
			reserva_pedido(request);				/*se o pedido for uma reserva ou prereserva, reserva o pedido*/
		}
		else if (request->tipo == 3){
			conf_prereserva(request); 				/*se for uma confirmação, confirma a prereserva*/
		}
		else if (request->tipo == 4){

			actualiza_pre(prereservas); 			/*atualiza as prereservas*/
			show_cinema();							/*se for um pedido para mostrar o cinema, mostra*/
		}
		else {
			cancelar_pedido(request);				/*se n for nenhum dos anteriores, cancela o pedido*/
			}

		sthread_sem_post(request->semaforo);		/*abre o semáforo do pedido*/
	}
	return NULL;
}


/*inicializa o servidor*/
int init_server(int num_server_threads){

	int i = 0;										
	reservas = lst_new();							/*cria lista de reservas*/
	prereservas = lst_new();						/*cria lista de prereservas*/
	
	Mutex1 = sthread_mutex_init();					/*cria o mutex*/
	Mutex2 = sthread_mutex_init();					/*cria o mutex*/
	sthread_t thr[num_server_threads];

	for (i=0; i<num_server_threads; i++) {
		thr[i]=sthread_create(trata_pedido, (void*)i, 1, 0);
		/*cria todas as threads*/
		if (thr[i] == NULL) {						/*se falhar a criação das threads*/
			printf("sthread_create failed\n");
			return -1;								/*retorna negativo*/
			}
			
		sthread_yield();
	}
	return 0;										/*bem sucedido*/
}
