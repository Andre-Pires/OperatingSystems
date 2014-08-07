/*
 * sthread_user.h - This file defines the user-level thread
 *                  implementation of sthreads. The routines
 *                  are all described in the sthread.h file.
 *
 */
#include <sthread_ctx.h>

#ifndef STHREAD_USER_H
#define STHREAD_USER_H 1

#define ST_ACTIVE 0
#define ST_WAITING 1
#define ST_SLEEPING 2
#define ST_BLOCKED 3
#define ST_DEAD    4

#define N_STATES 5

struct _sthread {
	sthread_ctx_t *saved_ctx;
	sthread_start_func_t start_routine_ptr;
	long wake_time;
	long time2wake;
	long vruntime;				/*Tempo de execução acumulado*/
	int remaining_time;
	int join_tid;
	void* join_ret;
	void* args;
	int tid;						/* id Thread */
	int prio;						/*Prioridade*/
	int nice;
	long statistics[N_STATES];

	// Suggestions for BFS
	long vdl;         
	int time_slice;

	int state;
	long last_clk_in_state;
  
	long start_clock;
};


/* Basic Threads */
void sthread_user_init(void *parameters);
sthread_t sthread_user_create(sthread_start_func_t start_routine, void *arg, int priority, int nice);
//sthread_t sthread_user_create(sthread_start_func_t start_routine, void *arg);
void sthread_user_exit(void *ret);
void sthread_user_yield(void);

/* Advanced Threads */
int sthread_user_sleep(int time);
int sthread_user_join(sthread_t thread, void **value_ptr);

int sthread_user_nice(int nice);
int sthread_user_dump();
void sthread_thread_dump(sthread_t thread);
long sthread_user_get_clock();
void sthread_update_stats(sthread_t thread, int newstate);

/* Synchronization Primitives */
sthread_mutex_t sthread_user_mutex_init(void);
void sthread_user_mutex_free(sthread_mutex_t lock);
void sthread_user_mutex_lock(sthread_mutex_t lock);
void sthread_user_mutex_unlock(sthread_mutex_t lock);

/* Semaphores primitives*/
sthread_sem_t sthread_user_sem_init (unsigned int initial_count);
void sthread_user_sem_wait (sthread_sem_t s);
void sthread_user_sem_post (sthread_sem_t s);
void sthread_user_sem_destroy (sthread_sem_t s);


/* ReadersWrite primitives*/

void sthread_user_rwlock_destroy(sthread_rwlock_t rwlock);

sthread_rwlock_t sthread_user_rwlock_init();

void sthread_user_rwlock_rdlock(sthread_rwlock_t rwlock);

void sthread_user_rwlock_wrlock(sthread_rwlock_t rwlock);

void sthread_user_rwlock_unlock(sthread_rwlock_t rwlock);

/************/



sthread_mon_t sthread_user_monitor_init();
void sthread_user_monitor_free(sthread_mon_t mon);
void sthread_user_monitor_enter(sthread_mon_t mon);
void sthread_user_monitor_exit(sthread_mon_t mon);
void sthread_user_monitor_wait(sthread_mon_t mon);
void sthread_user_monitor_signal(sthread_mon_t mon);
void sthread_user_monitor_signalall(sthread_mon_t mon);


#endif /* STHREAD_USER_H */
