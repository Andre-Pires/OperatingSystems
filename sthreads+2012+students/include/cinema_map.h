#ifndef CINEMA_MAP_H
#define CINEMA_MAP_H 1

#include <time.h>

/* Functions exposed to the servers to manipulate (read and update) the state of the cinema seats. The implementations of these functions include artificial delays that are used to simulate interactions with external devices (e.g. remote servers). 

	*** THESE FUNCTIONS MUST NOT BE USED BY THE CLIENT THREADS ***
	*** THESE FUNCTIONS CAN BE USED ONLY BY THE SERVER THREADS ***

	*** THESE FUNCTIONS DO NOT IMPLEMENT ANY SYNCHRONIZATION SCHEME ***
	*** CONCURRENT ACCESSES TO THESE FUNCTIONS MUST BE REGULATED ***
	*** BY AN EXTERNAL SYNCHRONIZATION SCHEME ***
*/



/* struct encapsulating the reservation status of a seat */

typedef struct status_t {
	int state; 		// 0=available; 1=pre-booked; 2=booked
	int id; 		// if state != 0 it stores the reservation id
	time_t exp_time; 	// if seat is pre-booked it stores the expiration time 
} status_t;




/* Used to create a new cinema map. It takes as input parameters*/
void init_backend_cinema(unsigned int num_rows, unsigned int num_cols);

/* Mark a seat (at a given row and column) as booked by the reservation having identifier id. Returns -1 if the coordinates passed as input are invalid; 0, otherwise.*/
int backend_reserva(unsigned int row, unsigned int col, int id);

/* Mark a seat (at a given row and column) as pre-booked by the reservation having identifier id and expiration time exp_time. Returns -1 if the coordinates passed as input are invalid; 0, otherwise.*/
int backend_prereserva(unsigned int row, unsigned int col, time_t exp_time, int id);

/* returns -1 if the reservation or pre-reservation does not currently exist; 0 if the reservation was removed and the correspondig seats made available */
int backend_cancela(unsigned int row, unsigned int col);

/* returns the status of the seat identified by the coordinates passed as input parameters; it returns a NULL pointer if the coordinates are invalid*/
status_t* backend_get_state(unsigned int row, unsigned int col);


#endif
