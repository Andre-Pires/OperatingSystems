#include <cinema.h>


/*inicializa o buffer*/
int init_buffer(int buf_size);

/*escreve pedido do cliente no buffer*/
int escreve_pedido(Pedido * lugares);

/*le pedido do cliente do buffer*/
Pedido * le_pedido();
