#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>

#define STACK_SIZE 4096


char stack3[STACK_SIZE];


typedef void * coroutine_t;

/* Quitte le contexte courant et charge les registres et la pile de CR. */
void enter_coroutine(coroutine_t cr);

/* Sauvegarde le contexte courant dans p_from, et entre dans TO. */
void switch_coroutine(coroutine_t *p_from, coroutine_t to);

/* Initialise la pile et renvoie une coroutine telle que, lorsqu’on entrera dedans,
elle commencera à s’exécuter à l’adresse initial_pc. */
coroutine_t init_coroutine(void *stack_begin, unsigned int stack_size, void (*initial_pc)(void)) {
  char *stack_end = ((char *)stack_begin) + stack_size;
  void **ptr = (void **)stack_end;
  /* ptr is the pointer past the allocation */
  ptr--;
  *ptr = initial_pc;
  ptr--;
  *ptr = stack_end - 20; /* rbp */;
  ptr-=5;
  return ptr;
}






struct thread_t {
  int ready; // 1 si le thread est prêt, 0 sinon
  coroutine_t coroutine;
};
//Underscore t pour définir que c'est un "type"

coroutine_t scheduling;
#define MAX_THREADS 10
struct thread_t globalThreads[MAX_THREADS];
int num_threads = 0;
int current_thread = -1;


struct semaphore_t {
  int blocked[MAX_THREADS]; // indices des threads bloqués
  int num_blocked_threads; // nombre d'entrées dans blocked
  int available; // nombre de resources dispo
}

void wait(struct semaphore_t *sem) {
  while(1) {
  if (sem->available) {
    sem->available--;
    return;
  } else {
    sem->blocked[sem->num_blocked_threads] = current_thread;
    sem->num_blocked_threads++;
    globalThreads[current_thread].ready = 0;
    yield();
  }
}
  // SI un octet est dispo:
  // le marquer comme consommé
  // rendre la main
  // SINON
  // s'incrire dans le sémaphore comme en attente
  // se marquer come non prêt
  // yield(); et recommencer jusqu'à ce qu'un octet soit dispo
}

void wakeup(struct semaphore_t *sem) {
  // SI des threads attendent dans sem
  // en réveiller un
  // marquer l'octet dispo
  // yield()
  // SINON, marquer  l'octet dispo
  if(sem->num_blocked_threads != 0){
    globalThreads[sem->blocked[sem->num_blocked_threads-1]].ready = 1;
    sem->num_blocked_threads--;
    sem->available++;
    yield();
  }
  else{
    sem->available++;
  }
  return;
}

void scheduler_fn() {
  while(1) {
    if (globalThreads[current_thread].ready==1) {
      fprintf(stderr, "switching to thread %d\n", current_thread);
      switch_coroutine(&scheduling, globalThreads[current_thread].coroutine);
    }
//    current_thread = (current_thread + 1) % num_threads;
    if (current_thread < num_threads - 1)
      current_thread++;
    else current_thread = 0;
   
  }
}

void yield() {
  switch_coroutine(&globalThreads[current_thread].coroutine, scheduling);
}


void thread_create(void (*f)(void)) {
  char* stack  = malloc(STACK_SIZE);
  assert(stack);
  globalThreads[num_threads].coroutine = init_coroutine(stack, STACK_SIZE, f);
  globalThreads[num_threads].ready = 1;
  num_threads++;
  assert(num_threads <= MAX_THREADS);
}

char byte_read[];
struct semaphore_t byte_available;

/* producteur */
void task1(void) {
  while(1) {
    int user_input = getchar();
    if (user_input != -1) {
      byte_read = (char)user_input;
      wakeup(&byte_available);
    }
    yield();
  }
}

/* consommateur 1 */
void task2(void) {
  while(1) {
    wait(&byte_available);
      putchar(byte_read);
    yield();
  }
}

/* consommateur 2 */
void task3 () {
   task2();
}

void task4 () {
  while (1) {
    printf("je suis là\n");
  }
}



int main() {
  // rendre getchar non bloquant
  fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
  
  
  scheduling = init_coroutine(stack3, STACK_SIZE, scheduler_fn);
  
  
  thread_create(task1);
  thread_create(task2);
  thread_create(task3);
   thread_create(task4);
  enter_coroutine(scheduling);
}

