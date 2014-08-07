#include <stdio.h>
#include <stdlib.h>
#include <sthread.h>
#include <semaphore.h>
#include <cinema.h>


/*declaração de variáveis globais*/
Pedido **buffer;									/*vetor de pedidos*/
sthread_sem_t sem_clientes;							/*semáforo das clientes*/
sthread_sem_t sem_servidoras;						/*semáforo das servidoras*/
sthread_mutex_t Mutex;
int id_counter = 1;									/*contador de id's*/
int BUFFER_SIZE;									/*tamanho do buffer*/
int p_clientes = 0;									/*ponteiro das clientes*/
int p_servidoras = 0;								/*ponteiro das servidoras*/


/*inicializa o buffer*/
int init_buffer(int buf_size){
	
	BUFFER_SIZE = buf_size;							/*define o buffer size como o que recebe*/
	buffer = (Pedido **)malloc(sizeof(Pedido *)*buf_size);

	p_clientes = 0;									/*poe os ponteiros a apontar para a primeira posição*/
	p_servidoras = 0;

	Mutex = sthread_mutex_init();					/*inicializa Mutex*/
	sem_clientes  = sthread_sem_init(buf_size);		/*inicializa semáforos*/
	sem_servidoras = sthread_sem_init(0);

	return 0;										/*inicializou devidamente*/
}


/*escreve pedido do cliente no buffer*/
int escreve_pedido(Pedido * request){

	sthread_sem_wait(sem_clientes);					/*espera pelo semáforo de clientes*/
	sthread_mutex_lock(Mutex);						/*quando o semáforo abre, tranca o mutex*/

	buffer[p_clientes] = request;					/*coloca o pedido no buffer*/
	p_clientes = (p_clientes+1) % BUFFER_SIZE;		/*ponteiro de clientes a apontar p a posição seguinte do buffer*/

	sthread_mutex_unlock(Mutex);					/*destranca o mutex*/
	sthread_sem_post(sem_servidoras);				/*abre o semáforo das servidoras*/

	return 0;
}


/*le pedido do cliente do buffer*/
Pedido * le_pedido(){

	Pedido * request;								/*cria um pedido 'request'*/
	
	sthread_sem_wait(sem_servidoras);				/*espera pelo semáforo das servidoras*/
	sthread_mutex_lock(Mutex);						/*quando o semáforo abre, tranca o mutex*/

	request = buffer[(p_servidoras)];				/*vai buscar o pedido ao buffer*/
	p_servidoras = (p_servidoras+1) % BUFFER_SIZE;	/*ponteiro das servidoras a apontar p a posição seguinte do buffer*/

	if(request->tipo == 1 || request->tipo == 2){	/*se for uma reserva ou uma prereserva*/
		request->id = id_counter;					/*cria um id unico para ela*/
		id_counter++;
	}

	sthread_mutex_unlock(Mutex);					/*destranca o mutex*/
	sthread_sem_post(sem_clientes);					/*abre o semáforo das servidoras*/

	return request;									/*devolve o pedido*/
}
