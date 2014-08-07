#include <cinema_map.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sthread.h>

#define DELAY 0
#define DELAY_TIME 1000

status_t** _c=NULL;
unsigned int _num_rows, _num_cols;

time_t init_time=0;
/* Used to create a new cinema map. It takes as input parameters*/

void init_backend_cinema(unsigned int num_rows, unsigned int num_cols) {
	int i,j;

	#ifdef DELAY
	sthread_sleep(DELAY_TIME);
	#endif

	_c = calloc(num_rows,sizeof(status_t*));
	for(i = 0;i<num_rows;i++) 
	_c[i]=calloc(num_cols,sizeof(status_t));

	for(i = 0;i<num_rows;i++)
		for (j=0; j<num_cols; j++) {
			_c[i][j].state=0;
			_c[i][j].id=-1;
			_c[i][j].exp_time=init_time;
		}

	_num_rows=num_rows;
	_num_cols=num_cols;
        #ifdef DEBUG
		printf("Cinema created of size %dx%d.\n",_num_rows,_num_cols);
	#endif
}

/* Mark a seat (at a given row and column) as booked by the reservation having identifier id. Returns -1 if the coordinates passed as input are invalid; 0, otherwise.*/
int backend_reserva(unsigned int row, unsigned int col, int id) {
	#ifdef DELAY
	sthread_sleep(DELAY_TIME);
	#endif

	if (_c==NULL || row>=_num_rows || col>=_num_cols) 
		return -1;

	_c[row][col].state=2;
	_c[row][col].id=id;
	_c[row][col].exp_time=init_time;

	return 0;
}

/* Mark a seat (at a given row and column) as pre-booked by the reservation having identifier id and expiration time exp_time. Returns -1 if the coordinates passed as input are invalid; 0, otherwise.*/
int backend_prereserva(unsigned int row, unsigned int col, time_t exp_time, int id) {
	#ifdef DELAY
	sthread_sleep(DELAY_TIME);
	#endif

	if (_c==NULL || row>=_num_rows || col>=_num_cols) 
		return -1;

	_c[row][col].state=1;
	_c[row][col].id=id;
	_c[row][col].exp_time=exp_time;

	return 0;
}

/* returns -1 if the reservation or pre-reservation does not currently exist; 0 if the reservation was removed and the correspondig seats made available */
int backend_cancela(unsigned int row, unsigned int col) {
	#ifdef DELAY
	sthread_sleep(DELAY_TIME);
	#endif

	if (_c==NULL || row>=_num_rows || col>=_num_cols) 
		return -1;

	_c[row][col].state=0;
	_c[row][col].id=-1;
	_c[row][col].exp_time=init_time;

	return 0;
}

/* returns the status of the seat identified by the coordinates passed as input parameters; it returns a NULL pointer if the coordinates are invalid*/
status_t* backend_get_state(unsigned int row, unsigned int col) {
	status_t* s;

	#ifdef DELAY
	sthread_sleep(DELAY_TIME);
	#endif

	if (_c==NULL || row>=_num_rows || col>=_num_cols) 
		return NULL;

	s=(status_t*) malloc(sizeof(status_t));
	*s=_c[row][col];

	return s;

}



