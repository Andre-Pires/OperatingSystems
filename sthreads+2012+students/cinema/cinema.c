#include <stdio.h>
#include <stdlib.h>
#include <sthread.h>
#include <semaphore.h>
#include <cinema.h>
#include <buffer.h>
#include <servidor.h>
#include <cinema_map.h>

/*inicializa a variável global*/
int cinema_inicializado = 0;

/*inicializa o cinema*/
int init_cinema(unsigned int num_rows, unsigned int num_cols, int num_server_threads, int buf_size) {
	
	int resultado = 0;

	init_backend_cinema(num_rows, num_cols); 		/*inicializa a sala de cinema*/
	init_buffer(buf_size);							/*inicializa o buffer*/
	init_matrix(num_rows, num_cols);				/*inicializa matriz de apoio*/
	resultado = init_server(num_server_threads);	/*inicializa servidores*/
	
	if (resultado == -1) return -1;					/*se n for bem sucedido retorna negativo*/
	if (cinema_inicializado == -1) return -1;		/*se o cinema já tiver sido inicializado, falha*/
	cinema_inicializado = -1;						/*marca o cinema como inicalizado*/
	
	return 0;										/*cinema iniciado com sucesso*/
}


/*faz a reserva por parte do cliente*/
int reserva(Coords_t* seats,int dim){

	Pedido * request;								/*cria um pedido 'request'*/
	request = (Pedido *)malloc(sizeof(Pedido));

	request->lugares = seats;						/*define os lugares do pedido como sendo os que recebe*/
	request->dimensao = dim;						/*define a dimensão dos lugares do pedido como sendo a que recebe*/
	request->resultado = 0;							/*define resultado como 0 porque ainda n há resposta*/
	request->tempo_vida = -1;						/*define o tempo de reserva como -1 pq n tem tempo de vida*/
	request->tipo = 1;								/*define tipo como 1 pq é o tipo predefinido p reserva*/
	request->semaforo = sthread_sem_init(0);		/*inicializa semáforo do pedido a 0*/

	escreve_pedido(request);						/*escreve o pedido no buffer*/
	sthread_sem_wait(request->semaforo);			/*espera pelo semáforo para avançar*/

	if (request->resultado == 0)					/*se for bem sucedido*/
		return request->id;							/*retorna o id do pedido*/
	else return -1;									/*se n for bem sucedido, retorna negativo*/ 
}


/*faz prereserva por parte do cliente*/
int prereserva(Coords_t* seats,int dim, int segundos) {

	Pedido * request;								/*cria um pedido 'request'*/
	request = (Pedido *)malloc(sizeof(Pedido));

	request->lugares = seats;						/*define os lugares do pedido como sendo os que recebe*/
	request->dimensao = dim;						/*define a dimensão dos lugares do pedido como sendo a que recebe*/
	request->resultado = 0;							/*define resultado como 0 porque ainda n há resposta*/
	request->tempo_vida = segundos;					/*define o tempo de duração da prereserva*/
	request->tipo = 2;								/*define tipo como 2 pq é o tipo predefinido p prereserva*/
	request->semaforo = sthread_sem_init(0);		/*inicializa semáforo do pedido a 0*/

	escreve_pedido(request);						/*escreve o pedido no buffer*/
	sthread_sem_wait(request->semaforo);			/*espera pelo semáforo para avançar*/

	if (request->resultado == 0)					/*se for bem sucedido*/
		return request->id;							/*retorna o id do pedido*/
	else return -1;									/*se n for bem sucedido, retorna negativo*/ 
}


/*confirma a prereserva por parte do cliente*/
int confirma_prereserva(int reservation_id){

	Pedido * request;								/*cria um pedido 'request'*/
	request = (Pedido *)malloc(sizeof(Pedido));

	request->lugares = NULL;						/*inicializa campos desconhecidos*/
	request->dimensao = 0;
	request->resultado = 0;
	request->tempo_vida = 0;
	request->id = reservation_id;					/*define o id do pedido como sendo o id da prereserva*/
	request->tipo = 3; 								/*define tipo como 3 porque é o tipo predefinido para confirmar prereserva*/
	request->semaforo = sthread_sem_init(0);		/*inicializa semáforo do pedido a 0*/
	
	escreve_pedido(request);						/*escreve o pedido no buffer*/
	sthread_sem_wait(request->semaforo);			/*espera pelo semáforo para avançar*/

	if (request->resultado == 0)					/*se for bem sucedido*/
		return 0;									/*retorna 0 se bem sucedido*/
	else return -1; 								/*se n for bem sucedido, retorna negativo*/
}


/*cancela reserva/prereserva por parte do cliente*/
int cancela(int reservation_id){
	
	Pedido * request;								/*cria um pedido 'request'*/
	request = (Pedido *)malloc(sizeof(Pedido));

	request->lugares = NULL;						/*inicializa campos desconhecidos*/
	request->dimensao = 0;
	request->resultado = 0;
	request->tempo_vida = 0;
	request->id = reservation_id;					/*define o id do pedido como sendo o id da reserva*/
	request->tipo = 0; 								/*define tipo como 0 porque é o tipo predefinido para cancelamento*/
	request->semaforo = sthread_sem_init(0);		/*inicializa semáforo do pedido a 0*/
	
	escreve_pedido(request);						/*escreve o pedido no buffer*/
	sthread_sem_wait(request->semaforo);			/*espera pelo semáforo para avançar*/

	if (request->resultado == 0)					/*se for bem sucedido*/
		return 0;									/*retorna 0 se bem sucedido*/
	else return -1; 								/*se n for bem sucedido, retorna negativo*/
}


/*pede para mostrar cinema por parte do cliente*/
void mostra_cinema() {
	Pedido * request;								/*cria um pedido 'request'*/
	request = (Pedido *)malloc(sizeof(Pedido));

	request->lugares = NULL;						/*inicializa campos desconhecidos*/
	request->dimensao = 0;
	request->resultado = 0;
	request->tempo_vida = 0;
	request->id = 0;
	request->tipo = 4; 								/*define tipo como 4 porque é o tipo predefinido para mostrar cinema*/
	request->semaforo = sthread_sem_init(0);		/*inicializa semáforo do pedido a 0*/
	
	escreve_pedido(request);						/*escreve o pedido no buffer*/
	sthread_sem_wait(request->semaforo);
} 
