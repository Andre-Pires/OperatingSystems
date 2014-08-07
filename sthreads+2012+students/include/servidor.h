#include <list.h>


/*inicializa matriz auxiliar (baseada na inicialização do mapa)*/
void init_matrix(int num_rows, int num_cols);

/*compara os lugares (função para qsort)*/
int compara(const void * a, const void * b);

/*ordena os lugares do pedido*/
int ordena(Pedido * request);

/*tranca (individualmente) os lugares que recebe, na matriz auxiliar*/
void lock(Coords_t * lugares, int dimensao);

/*destranca (individualmente) os lugares que recebe, na matriz auxiliar*/
void unlock(Coords_t * lugares, int dimensao);

/*marca os lugares de um determinado pedido, no mapa do cinema*/
void marca_lugares(Pedido * request);

/*cancela os lugares do pedido*/
void cancela_lugares(Pedido * request, Pedido * backup);

/*reserva o pedido recebido*/
int reserva_pedido(Pedido * request);

/*cancela o pedido recebido*/
int cancelar_pedido(Pedido * request);

/*atualiza a lista de prereservas*/
void actualiza_pre(list_t * prereservas);

/*confirma uma prereserva*/
int conf_prereserva(Pedido * request);

/*mostra o mapa do cinema, de acordo com o indicado em 'show.c'*/
void show_cinema();

/*trata o pedido do buffer*/
void * trata_pedido(void * arg);

/*inicializa o servidor*/
int init_server(int num_server_threads);
