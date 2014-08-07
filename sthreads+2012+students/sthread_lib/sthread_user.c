/* Simplethreads Instructional Thread Package
 * 
 * sthread_user.c - Implements the sthread API using user-level threads.
 *
 *    You need to implement the routines in this file.
 *
 * Change Log:
 * 2002-04-15        rick
 *   - Initial version.
 * 2005-10-12        jccc
 *   - Added semaphores, deleted conditional variables
 */

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "sthread.h"
#include "sthread_user.h"
#include "sthread_ctx.h"
#include "sthread_time_slice.h"
#include "queue.h"
#include "generic_queue.h"

/*listas de threads*/
static queue_t **exe_thr_array;			/*listas de threads executaveis*/
static queue_t *dead_thr_list;        	/*lista de threads "mortas"*/
static queue_t *sleep_thr_list;			/*lista de threads "adormecidas"*/
static queue_t *join_thr_list;			/*lista de threads em espera*/
static queue_t *zombie_thr_list;		/*lista de threads "zombie"*/
static gqueue_t *mutex_list;			/*lista de trincos*/
static gqueue_t *monitor_list;			/*lista de monitores*/

static struct _sthread *active_thr;   	/*thread activa*/
static int tid_gen;                   	/*gerador de tid's*/
#define MAX3 3							/*constante auxiliar de valor 3*/
#define EXE_DIM 6						/*dimensao do vector de exe thr*/

#define CLOCK_TICK 10000				/*clock tick - 10ms*/
static long Clock;
static int TIMESLICE;					


/*********************************************************************/
/* FUNCOES AUXILIARES                                                */
/*********************************************************************/

/*actualiza o time_slice (remaining_time) de uma thread t*/
void actualiza_time_slice(sthread_t t){
	t->time_slice--;
	t->last_clk_in_state = Clock;
}

/*actualiza o wait_time de uma thread t*/
void actualiza_WaiT(sthread_t t){
    int aux;
    aux = Clock - t->last_clk_in_state;

    t->statistics[ST_WAITING] += aux; 
    t->last_clk_in_state = Clock;
}

/*actualiza o sleep_time de uma thread t*/
void actualiza_SlpT(sthread_t t){
    int aux;
    aux = Clock - t->last_clk_in_state;

    t->statistics[ST_SLEEPING] += aux; 
    t->last_clk_in_state = Clock;
}

/*actualiza o blocked_time de uma thread t*/
void actualiza_BlkT(sthread_t t){
    int aux;
    aux = Clock - t->last_clk_in_state;

    t->statistics[ST_BLOCKED] += aux; 
    t->last_clk_in_state = Clock;
}

/*actualiza o runtime de uma thread t*/
void actualiza_RunT(sthread_t t){
    int aux;
    aux = Clock - t->last_clk_in_state;

    t->statistics[ST_ACTIVE] += aux; 
    t->last_clk_in_state = Clock;
}

/*actualiza o wait_time de todas as threads executaveis*/
void actualiza_exe_array(){
	queue_element_t *temp;
	struct _sthread *new_thread;
	int i;

	for (i = 0; i < EXE_DIM; ++i){
		temp = exe_thr_array[i]->first;
   		while(temp!=NULL){
   			new_thread = temp->thread;  		
   			actualiza_WaiT(new_thread);
   			if (new_thread->vdl != 0)
   				new_thread->vdl--;
   			temp=temp->next;
   		}
	}
}

/*guarda uma thread na lista de executaveis correspondente à sua prioridade*/
void guarda_thr(struct _sthread *thread){
	int p = thread->prio;
	
	if(p <= MAX3){
		//if(thread->time_slice == 0) 
			thread->time_slice = TIMESLICE; 
		queue_insert(exe_thr_array[p], thread);
	}
	else{
		//if(thread->time_slice == 0){
			thread->time_slice = TIMESLICE; 
			thread->vdl = Clock + ((thread->nice)+20)*TIMESLICE;
		//}
		queue_insert(exe_thr_array[p], thread);
	}
}

/*procura uma nova thread para utilizar como active_thr*/
void procura_thread(int prio){
	int i;
		
	for(i=0; i<prio; i++){
		if(i <= MAX3){
			if(!queue_is_empty(exe_thr_array[i])){
				active_thr = queue_remove(exe_thr_array[i]);
				break;
			}
		}else{ 
			if (!queue_is_empty(exe_thr_array[i])){
				active_thr = queue_vdl_rem(exe_thr_array[i]); //MUDAR PARA VDL
				break;
			}
		}
	}
}

/*[boolean]indica se todas as listas de threads executaveis estão vazias*/
int all_exe_empty(){
	int i=1;
	int p;
	
	for (p=0; p<EXE_DIM; p++){
		i = queue_is_empty(exe_thr_array[p]);
		if(i != 1) break;
	}
	return i;
}

/*preempção: verifica se existe alguma threads mais prioritária e troca-a*/
void preempcao(){
    int i;

    //printf("PREEMPCAO\n");
    for(i=0; i<(active_thr->prio); i++){
        if(!queue_is_empty(exe_thr_array[i])){
            sthread_user_yield();
            break;
        }
    }
}


/*********************************************************************/
/* Part 1: Creating and Scheduling Threads                           */
/*********************************************************************/

void sthread_user_free(struct _sthread *thread);
void mutex_dump();
void monitor_dump();
void sthread_user_dispatcher(void);
void sthread_user_yield(void);

void sthread_aux_start(void){
	splx(LOW);
 	active_thr->start_routine_ptr(active_thr->args);
  	sthread_user_exit((void*)0);
}

/*inicializa as threads*/
void sthread_user_init(void *parameters){
	
	int i;								/*variável auxiliar*/
	
	int *p = (int*)parameters;
	TIMESLICE = *p;						/*passagem do TIMESLICE como parametro*/
	
	/*alocação de memória para o vetor de listas de threads executaveis*/
	exe_thr_array = (queue_t **)malloc(sizeof(queue_t *)*EXE_DIM);

	/*criação das listas de threads*/
	for(i=0; i<EXE_DIM; i++)
		exe_thr_array[i] = create_queue();
		
	dead_thr_list = create_queue(); 
	sleep_thr_list = create_queue(); 
	join_thr_list = create_queue(); 
	zombie_thr_list = create_queue(); 
	mutex_list = create_gqueue(); 
	monitor_list = create_gqueue(); 
	
	tid_gen = 1;						/*inicialização do contador*/
	Clock = 1;							/*inicialização do clock*/

	/*criação de uma main_thread e inicialização dos valores da mesma*/
	struct _sthread *main_thread = malloc(sizeof(struct _sthread));
	main_thread->start_routine_ptr = NULL;
	main_thread->args = NULL;
	main_thread->saved_ctx = sthread_new_blank_ctx();
	main_thread->time_slice = TIMESLICE;
	main_thread->last_clk_in_state = Clock;
	main_thread->wake_time = 0;
	main_thread->join_tid = 0;
	main_thread->join_ret = NULL;
	main_thread->tid = tid_gen++;
	main_thread->prio = 4;
	main_thread->nice = 0;
	for(i=0;i<N_STATES;i++) main_thread->statistics[i]=0;
	
	/*se a prioridade for maior que 3, actualiza o vdl*/
	if (main_thread->prio > MAX3)
		main_thread->vdl = Clock + ((main_thread->nice)+20)*TIMESLICE;
	active_thr = main_thread;

	sthread_time_slices_init(sthread_user_dispatcher, CLOCK_TICK);
}

/*cria nova thread*/
sthread_t sthread_user_create(sthread_start_func_t start_routine, void *arg, int priority, int nice){
	
	/*criação de uma new_thread e inicialização dos valores da mesma*/
	struct _sthread *new_thread = malloc(sizeof(struct _sthread));
	sthread_ctx_start_func_t func = sthread_aux_start;
	new_thread->start_routine_ptr = start_routine;
	new_thread->args = arg;
	new_thread->saved_ctx = sthread_new_ctx(func);
	new_thread->time_slice = TIMESLICE;
	new_thread->wake_time = 0;
	new_thread->join_tid = 0;
	new_thread->join_ret = NULL;
	new_thread->last_clk_in_state = Clock;

	new_thread->prio = priority;
	new_thread->nice = nice;
	int i;
	for(i=0;i<N_STATES;i++) new_thread->statistics[i]=0;
	if (priority > MAX3)
		new_thread->vdl = Clock + (nice+20)*TIMESLICE;

	splx(HIGH);
	new_thread->tid = tid_gen++;	
	guarda_thr(new_thread);				/*guarda a nova thread na lista de executáveis correspondente*/
	splx(LOW);
	return new_thread;
}

/*termina a thread chamada*/
void sthread_user_exit(void *ret) {
   	struct _sthread *old_thr;

  	splx(HIGH);

	int is_zombie = 1;
   	int p;

   	/*desbloqueia threads à espera na lista join*/
   	queue_t *tmp_queue = create_queue();       
   	while (!queue_is_empty(join_thr_list)) {
      	struct _sthread *thread = queue_remove(join_thr_list);
     	
      	if (thread->join_tid == active_thr->tid) {
      		actualiza_BlkT(thread);
      		thread->join_ret = ret;
      		guarda_thr(thread);
         	is_zombie = 0;
      	} else {
         	queue_insert(tmp_queue,thread);
      	}
   	}

   	delete_queue(join_thr_list);
   	join_thr_list = tmp_queue;
 
   	if (is_zombie) {
      	queue_insert(zombie_thr_list, active_thr);
   	} else {
      	queue_insert(dead_thr_list, active_thr);
   	}
   
  	if(all_exe_empty()){  							/* pode acontecer se a unica thread em execucao fizer */ 
    	for (p=0; p<EXE_DIM; p++)
			free(exe_thr_array[p]);              	/* sthread_exit(0). Este codigo garante que o programa sai bem. */
    	delete_queue(dead_thr_list);
    	sthread_user_free(active_thr);
    	printf("Exec queue is empty!\n");
    	exit(0);
  	}

  
   	// remove from exec list
   	old_thr = active_thr;

   	procura_thread(EXE_DIM);
   
   	sthread_switch(old_thr->saved_ctx, active_thr->saved_ctx);

   	splx(LOW);
   	return;
}

/*trata do despacho das threads*/
void sthread_user_dispatcher(void){
	Clock++;
	queue_t *tmp_queue = create_queue(); 
	int i, p;

	while (!queue_is_empty(sleep_thr_list)) {
		struct _sthread *thread = queue_remove(sleep_thr_list);
      
		if (thread->wake_time == Clock) {
			thread->wake_time = 0;
			guarda_thr(thread);

		} else {
			actualiza_SlpT(thread);
			queue_insert(tmp_queue,thread);
		}
	}
	delete_queue(sleep_thr_list);
	sleep_thr_list = tmp_queue;

   	actualiza_exe_array();
 	
 	actualiza_RunT(active_thr);
 	actualiza_time_slice(active_thr);
 	
 	if(active_thr->time_slice == 0){
		sthread_user_yield();
	}

    preempcao();
}

/*troca a thread em execução*/
void sthread_user_yield(void){
  	struct _sthread *old_thr;

  	splx(HIGH);
  	
  	old_thr = active_thr;
  	guarda_thr(active_thr);
	procura_thread(EXE_DIM);
	sthread_switch(old_thr->saved_ctx, active_thr->saved_ctx);
	
	splx(LOW);
}

/*liberta memória utilizada para thread*/
void sthread_user_free(struct _sthread *thread){
  sthread_free_ctx(thread->saved_ctx);
  free(thread);
}


/*********************************************************************/
/* Part 2: Join and Sleep Primitives                                 */
/*********************************************************************/
int find_mutex(int id);

int sthread_user_join(sthread_t thread, void **value_ptr)
{
	/* suspends execution of the calling thread until the target thread
	 terminates, unless the target thread has already terminated.
	 On return from a successful pthread_join() call with a non-NULL 
	 value_ptr argument, the value passed to pthread_exit() by the 
	 terminating thread is made available in the location referenced 
	 by value_ptr. When a pthread_join() returns successfully, the 
	 target thread has been terminated. The results of multiple 
	 simultaneous calls to pthread_join() specifying the same target 
	 thread are undefined. If the thread calling pthread_join() is 
	 canceled, then the target thread will not be detached. 
	 
	 If successful, the pthread_join() function returns zero. 
	 Otherwise, an error number is returned to indicate the error. */

	splx(HIGH);
	// checks if the thread to wait is zombie
	int found = 0;
	int i;

	queue_t *tmp_queue = create_queue(); // Added in BFS
	while (!queue_is_empty(zombie_thr_list)) {
		struct _sthread *zthread = queue_remove(zombie_thr_list);
		if (thread->tid == zthread->tid) {
			*value_ptr = thread->join_ret;
			queue_insert(dead_thr_list,zthread);
			found = 1;
		} else {
			queue_insert(tmp_queue,zthread);
		}
	}
	delete_queue(zombie_thr_list);
	zombie_thr_list = tmp_queue;
	
	if (found) {
		splx(LOW);
		return 0;
	}
	
	// search active queue
	if (active_thr->tid == thread->tid) {
		found = 1;
	}
	
	queue_element_t *qe = NULL;
	
	 // search exe
	for (i=0; i<EXE_DIM; i++){
		if (!queue_is_empty(exe_thr_array[i])){
			qe = exe_thr_array[i]->first;
			while (!found && qe != NULL) {
	 			if (qe->thread->tid == thread->tid){ 
	 				found = 1;
	 				break;
	 			}
	 			qe = qe->next;
			}
			if (found) break;
		}
	}
	 
	// search sleep
	qe = sleep_thr_list->first;
	while (!found && qe != NULL) {
		if (qe->thread->tid == thread->tid) {
			found = 1;
		}
		qe = qe->next;
	}
	
	// search join
	qe = join_thr_list->first;
	while (!found && qe != NULL) {
		if (qe->thread->tid == thread->tid) {
			found = 1;
		}
		qe = qe->next;
	}

	if(!found && find_mutex(thread->tid)) found = 1;
	
	// if found blocks until thread ends
	if (!found) {
		splx(LOW);
		return -1;
	} else {
		active_thr->join_tid = thread->tid;
		
		struct _sthread *old_thr = active_thr;

		queue_insert(join_thr_list, old_thr);
		procura_thread(EXE_DIM); // Remove in BFS

		sthread_switch(old_thr->saved_ctx, active_thr->saved_ctx);
		*value_ptr = thread->join_ret;
	}
	
	splx(LOW);
	return 0;
}

/* minimum sleep time of 1 clocktick.
  1 clock tick, value 10 000 = 10 ms */

/*adormece uma thread por 'time' tempo*/
int sthread_user_sleep(int time)
{
   splx(HIGH);
   
   int i;
   long num_ticks = time / CLOCK_TICK;
   if (num_ticks == 0) {
      splx(LOW);
      
      return 0;
   }
   
   active_thr->wake_time = Clock + num_ticks;

   queue_insert(sleep_thr_list,active_thr); 

   if(all_exe_empty()){  							/* pode acontecer se a unica thread em execucao fizer */
       for(i=0; i<EXE_DIM; i++)
            free(exe_thr_array[i]);              	/* sleep(x). Este codigo garante que o programa sai bem. */
       delete_queue(dead_thr_list);
       sthread_user_free(active_thr);
       printf("Exec queue is empty! too much sleep...\n");
       exit(0);
   }

   	sthread_t old_thr = active_thr;
    procura_thread(EXE_DIM); 
   	sthread_switch(old_thr->saved_ctx, active_thr->saved_ctx);
   
   splx(LOW);
   return 0;
}

int sthread_user_nice(int nice){
	
	int new_nice;
	
	new_nice = active_thr->nice; 
	active_thr->nice = nice;		// novo nice

	return new_nice;
}

void sthread_thread_dump(sthread_t thread){
  printf("TID:%02d\tPRIO:%05d\tNICE:%05d\tWAKE:%05ld\n",
    thread->tid,
    thread->prio,
    thread->nice,
    thread->wake_time);
  sthread_update_stats(thread, thread->state);
  printf("\tVDL :%05ld\tT_SL:%05d\tRunT:%05ld\n",
    thread->vdl,
    thread->time_slice,
    thread->statistics[ST_ACTIVE]);
  printf("\tWaiT:%05ld\tSlpT:%05ld\tBlkT:%05ld\n",
    thread->statistics[ST_WAITING],
    thread->statistics[ST_SLEEPING],
    thread->statistics[ST_BLOCKED]);
   
}

void sthread_update_stats(sthread_t thread, int newstate){
}

int sthread_user_dump(){
	int i;
    printf("<<Active Thread>>\n");
    sthread_thread_dump(active_thr);
    for(i=0; i<EXE_DIM; i++)
    	queue_dump(exe_thr_array[i],"\n<<Executable Queue>>"); 
    queue_dump(sleep_thr_list,"\n<<Sleep Queue>>");
    mutex_dump();
//    monitor_dump();
    queue_dump(dead_thr_list,"\n<<Dead Queue>>");
    queue_dump(zombie_thr_list,"\n<<Zombie Queue>>");
    queue_dump(join_thr_list,"\n<<Join Queue>>");
    return 0;
}

long sthread_user_get_clock(){
  return Clock;
}


/* --------------------------------------------------------------------------*
 * Synchronization Primitives                                                *
 * ------------------------------------------------------------------------- */

/*
 * Mutex implementation
 */

struct _sthread_mutex
{
    lock_t l;
    struct _sthread *thr;
    queue_t* queue;
};

void mutex_dump(){
    char buf[128];
  
    void *temp = gqueue_first(mutex_list);
    while(temp!=NULL){
        if(((sthread_mutex_t)temp)->thr != NULL)
            sprintf(buf,"\n<<ON MUTEX LOCKED BY TID:%d>>",((sthread_mutex_t)temp)->thr->tid);
        else
            sprintf(buf,"\n<<ON MUTEX LOCKED BY NOONE!!!>>");
        queue_dump(((sthread_mutex_t)temp)->queue,buf);
        temp = gqueue_find_next(mutex_list,temp);
    }

}

int find_mutex(int id) {
    queue_element_t *qe;
    sthread_mutex_t temp = (sthread_mutex_t)gqueue_first(mutex_list);
    while(temp!=NULL){
        qe = temp->queue->first;
        while (qe != NULL) {
            if (qe->thread->tid == id) {
            return 1;
        }
        qe = qe->next;
    }
    temp = gqueue_find_next(mutex_list,temp);
   }
   return 0;
}

sthread_mutex_t sthread_user_mutex_init()
{
    sthread_mutex_t lock;

    if(!(lock = malloc(sizeof(struct _sthread_mutex)))){
        printf("Error in creating mutex\n");
        return 0;
    }

    /* mutex initialization */
    lock->l=0;
    lock->thr = NULL;
    lock->queue = create_queue(); 
  
    gqueue_insert(mutex_list, (void *)lock);
    return lock;
}

void sthread_user_mutex_free(sthread_mutex_t lock)
{
    gqueue_find_remove(mutex_list, (void *)lock);
    delete_queue(lock->queue);
    free(lock);
}

void sthread_user_mutex_lock(sthread_mutex_t lock)
{
    while(atomic_test_and_set(&(lock->l))) {}

    if(lock->thr == NULL){
        lock->thr = active_thr;

        atomic_clear(&(lock->l));
    } else {
        queue_insert(lock->queue, active_thr);
    
        atomic_clear(&(lock->l));

        splx(HIGH);
        struct _sthread *old_thr;
        old_thr = active_thr;
        procura_thread(EXE_DIM);
        sthread_switch(old_thr->saved_ctx, active_thr->saved_ctx);
    
        splx(LOW);
    }
}

void sthread_user_mutex_unlock(sthread_mutex_t lock)
{
    if(lock->thr!=active_thr){
        printf("unlock without lock! ctid:%d ltid:%d\n",active_thr->tid,lock->thr->tid);
        return;
    }

    while(atomic_test_and_set(&(lock->l))) {}

    if(queue_is_empty(lock->queue)){
        lock->thr = NULL;
    } else {
        lock->thr = queue_remove(lock->queue);
        guarda_thr(lock->thr);
 	}

    atomic_clear(&(lock->l));
    preempcao(); // Added for preemption
}

/*------------
 Semaphore Implementation 
*/

struct _sthread_sem {
    unsigned int count;
    sthread_mutex_t mutex;
};

sthread_sem_t sthread_user_sem_init (unsigned int initial_count) {
  sthread_sem_t sem;
  
  if(!(sem = malloc(sizeof(struct _sthread_sem)))){
    printf("Error creating semaphore\n");
    return 0;
  }
  sem->count = initial_count;
  sem->mutex = sthread_user_mutex_init();
  return sem;
}

void sthread_user_sem_wait (sthread_sem_t s) {
  	while(atomic_test_and_set(&(s->mutex->l))) {}
    
  	if(s->count>0){
    	s->count--;
    	atomic_clear(&(s->mutex->l));
  	} else {
    	queue_insert(s->mutex->queue, active_thr);
    
    atomic_clear(&(s->mutex->l));
    
    splx(HIGH);
    struct _sthread *old_thr;
    old_thr = active_thr;
    procura_thread(EXE_DIM);
    sthread_switch(old_thr->saved_ctx, active_thr->saved_ctx);
    
    splx(LOW);
  }
}

void sthread_user_sem_post (sthread_sem_t s) {
    
  	while(atomic_test_and_set(&(s->mutex->l))) {}
    
  	if(queue_is_empty(s->mutex->queue)){
    	s->count++;
  	} else {
    	s->mutex->thr = queue_remove(s->mutex->queue);
    	guarda_thr(s->mutex->thr);
  	}
    
  	atomic_clear(&(s->mutex->l));
  	preempcao();
 }

void sthread_user_sem_destroy (sthread_sem_t s){
    sthread_user_mutex_free(s->mutex);
    free(s);
}

/*
 * Readers/Writer implementation
 */

struct _sthread_rwlock {
    int nleitores; 
    int /*boolean_t*/ em_escrita;
    int leitores_espera;
    int escritores_espera;
    sthread_mutex_t m;
    sthread_sem_t leitores;
    sthread_sem_t escritores;
};

void sthread_user_rwlock_destroy(sthread_rwlock_t rwlock){

    sthread_user_mutex_free(rwlock->m);
    sthread_user_sem_destroy(rwlock->leitores);
    sthread_user_sem_destroy(rwlock->escritores);
    free(rwlock);

}

sthread_rwlock_t sthread_user_rwlock_init(){

    sthread_rwlock_t rwlock;

  if(!(rwlock = malloc(sizeof(struct _sthread_rwlock)))){
    printf("Error in creating rwlock\n");
    return 0;
  }

    rwlock->leitores = sthread_user_sem_init(0) ;
    rwlock->escritores = sthread_user_sem_init(0);
    rwlock->m = sthread_user_mutex_init();
    rwlock->nleitores=0;
    rwlock->em_escrita=0; /*FALSE;*/
    rwlock->leitores_espera=0;
    rwlock->escritores_espera=0;
  
    return rwlock;

}

void sthread_user_rwlock_rdlock(sthread_rwlock_t rwlock){

    sthread_user_mutex_lock(rwlock->m);
    if (rwlock->em_escrita || rwlock->escritores_espera > 0) {
	rwlock->leitores_espera++;
	sthread_user_mutex_unlock(rwlock->m);
	sthread_user_sem_wait(rwlock->leitores);
	sthread_user_mutex_lock(rwlock->m);
    }
    else{
	rwlock->nleitores++;
    }
    sthread_user_mutex_unlock(rwlock->m);

}

void sthread_user_rwlock_wrlock(sthread_rwlock_t rwlock){

    sthread_user_mutex_lock(rwlock->m);
    if (rwlock->em_escrita || rwlock->nleitores > 0) {
	rwlock->escritores_espera++;
	sthread_user_mutex_unlock(rwlock->m);
	sthread_user_sem_wait(rwlock->escritores);
	sthread_user_mutex_lock(rwlock->m);
    }
    else{
	rwlock->em_escrita = 1; /*TRUE;*/
    }
    sthread_user_mutex_unlock(rwlock->m);   
   
}


void sthread_user_rwlock_unlock(sthread_rwlock_t rwlock){
    int i;
    sthread_user_mutex_lock(rwlock->m);
    
    if (/*TRUE*/ 1== rwlock->em_escrita) { /* writer unlock*/
	
	rwlock->em_escrita = 0; /*FALSE; */
	if (rwlock->leitores_espera > 0){
	    for (i=0; i< rwlock->leitores_espera; i++) {
		sthread_user_sem_post(rwlock->leitores);
		rwlock->nleitores++;
	    }
	    rwlock->leitores_espera -= i;
        }else{
           if (rwlock->escritores_espera > 0) {
	     sthread_user_sem_post(rwlock->escritores);
	     rwlock->em_escrita=1; /*TRUE;*/
	     rwlock->escritores_espera--;
	    }
	}
    }else{ /* reader unlock*/
	
	rwlock->nleitores--;
	if (rwlock->nleitores == 0 && rwlock->escritores_espera > 0){
	    sthread_user_sem_post(rwlock->escritores);
	    rwlock->em_escrita=1; /*TRUE;*/
	    rwlock->escritores_espera--;
	}
    }
    sthread_user_mutex_unlock(rwlock->m);    
}

/*
 * Monitor implementation
 */

struct _sthread_mon {
 	sthread_mutex_t mutex;
	queue_t* queue;
};

void monitor_dump(){
  
  void *temp = gqueue_first(monitor_list);
  while(temp!=NULL){
    queue_dump(((sthread_mon_t)temp)->queue,"<<ON MONITOR>>");
    temp = gqueue_find_next(mutex_list,temp);
  }

}


sthread_mon_t sthread_user_monitor_init()
{
  sthread_mon_t mon;
  if(!(mon = malloc(sizeof(struct _sthread_mon)))){
    printf("Error creating monitor\n");
    return 0;
  }

  mon->mutex = sthread_user_mutex_init();
  mon->queue = create_queue(0); 
  gqueue_insert(monitor_list, (void *)mon);
  return mon;
}

void sthread_user_monitor_free(sthread_mon_t mon)
{
  sthread_user_mutex_free(mon->mutex);
  gqueue_find_remove(monitor_list, (void *)mon);
  delete_queue(mon->queue);
  free(mon);
}

void sthread_user_monitor_enter(sthread_mon_t mon)
{
  sthread_user_mutex_lock(mon->mutex);
}

void sthread_user_monitor_exit(sthread_mon_t mon)
{
  sthread_user_mutex_unlock(mon->mutex);
}

void sthread_user_monitor_wait(sthread_mon_t mon)
{
  struct _sthread *temp;

  if(mon->mutex->thr != active_thr){
    printf("monitor wait called outside monitor\n");
    return;
  }

  /* inserts thread in queue of blocked threads */
  temp = active_thr;
  queue_insert(mon->queue, temp);

  /* exits mutual exclusion region */
  sthread_user_mutex_unlock(mon->mutex);

  splx(HIGH);
  struct _sthread *old_thr;
  old_thr = active_thr;
  procura_thread(EXE_DIM); 
  sthread_switch(old_thr->saved_ctx, active_thr->saved_ctx);
  splx(LOW);
}

void sthread_user_monitor_signal(sthread_mon_t mon)
{
  struct _sthread *temp;

  if(mon->mutex->thr != active_thr){
    printf("monitor signal called outside monitor\n");
    return;
  }

  while(atomic_test_and_set(&(mon->mutex->l))) {}
  if(!queue_is_empty(mon->queue)){
    /* changes blocking queue for thread */
    temp = queue_remove(mon->queue);
    queue_insert(mon->mutex->queue, temp);
  } else
    printf("queue is empty\n");
  atomic_clear(&(mon->mutex->l));
}

void sthread_user_monitor_signalall(sthread_mon_t mon)
{
  struct _sthread *temp;

  if(mon->mutex->thr != active_thr){
    printf("monitor signalall called outside monitor\n");
    return;
  }

  while(atomic_test_and_set(&(mon->mutex->l))) {}
  while(!queue_is_empty(mon->queue)){
    /* changes blocking queue for thread */
    temp = queue_remove(mon->queue);
    queue_insert(mon->mutex->queue, temp);
  }
  atomic_clear(&(mon->mutex->l));
}


/* The following functions are dummies to 
 * highlight the fact that pthreads do not
 * include monitors.
 */

sthread_mon_t sthread_dummy_monitor_init()
{
   printf("WARNING: pthreads do not include monitors!\n");
   return NULL;
}


void sthread_dummy_monitor_free(sthread_mon_t mon)
{
   printf("WARNING: pthreads do not include monitors!\n");
}


void sthread_dummy_monitor_enter(sthread_mon_t mon)
{
   printf("WARNING: pthreads do not include monitors!\n");
}


void sthread_dummy_monitor_exit(sthread_mon_t mon)
{
   printf("WARNING: pthreads do not include monitors!\n");
}


void sthread_dummy_monitor_wait(sthread_mon_t mon)
{
   printf("WARNING: pthreads do not include monitors!\n");
}


void sthread_dummy_monitor_signal(sthread_mon_t mon)
{
   printf("WARNING: pthreads do not include monitors!\n");
}

void sthread_dummy_monitor_signalall(sthread_mon_t mon)
{
   printf("WARNING: pthreads do not include monitors!\n");
}