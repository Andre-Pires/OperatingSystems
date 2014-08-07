#include <stdio.h>
#include <cinema_map.h>
#include <sthread.h>
#include <cinema.h>
#include <time.h>


#define NUM_ROWS 10
#define NUM_COLUMNS 10
#define PADDING 4

	// example cinema state:
	// - assume 1 reservation with id 7 for 2 seats : {[0,0], [0,1]}
	// - assume 2 prereservations, with ids 3 and 4, prereserving 1 seat each: {[2,2]} , {[3,3]}
	// - assume there were 3 expired prereservations (not confirmed in time)


int main() {
	int i,j;

	//init backend and prefill with the example cinema state
	init_backend_cinema(NUM_ROWS,NUM_COLUMNS);
	backend_reserva(0,0,7);
	backend_reserva(0,1,7);

	backend_prereserva(2,2,time(NULL),3);
	backend_prereserva(3,3,time(NULL),4);

	//print map state
  	for (j=0; j<NUM_COLUMNS*(PADDING+1); j++) 
		printf("-");

	printf("\n");


	for(i = 0;i<NUM_ROWS;i++) {
		printf("|");
		for (j=0; j<NUM_COLUMNS; j++) {
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

  	for (j=0; j<NUM_COLUMNS*(PADDING+1); j++) 
		printf("-");

	printf("\n");

	//print map state - including res_id in case seat is (pre-)reserved

	for(i = 0;i<NUM_ROWS;i++) {
		printf("<");
		for (j=0; j<NUM_COLUMNS; j++) {
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


	int length_res=1;
	int length_preres=2;
	int expired=3; // expired prereservations

	printf("#res:%d\n",length_res);
	printf("#preres:%d\n",length_preres);
	printf("#expired:%d\n",expired);

	printf("-Reservations:\n");
	printf("{ Res:7 #seats:2 - { [0,0][0,1] }\n");

	printf("-Prereservations:\n");
	printf("{ Preres:3 #seats:1 - { [2,2] }\n");
	printf("{ Preres:4 #seats:1 - { [3,3] }\n");
	
	// if there are no reservations or preservations the output must be:
	// printf("-Reservations:\n"); or printf("-Prereservations:\n");
	// printf("Empty list.\n");
		
	return 0;
}
