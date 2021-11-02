#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <signal.h>

#define STACK_SIZE 4096


char stack3[STACK_SIZE]__attribute__((aligned(4096)));



typedef void * coroutine_t;

/* Quitte le contexte courant et charge les registres et la pile de CR. */
void enter_coroutine(coroutine_t cr);

/* Sauvegarde le contexte courant dans p_from, et entre dans TO. */
void switch_coroutine(coroutine_t *p_from, coroutine_t to);

/* Initialise la pile et renvoie une coroutine telle que, lorsqu’on entrera dedans,
elle commencera à s’exécuter à l’adresse initial_pc. */
coroutine_t init_coroutine(void *stack_begin, unsigned int stack_size, void (*initial_pc)(void)) {
  assert(((intptr_t)stack_begin) % 4096 == 0);
  char *stack_end = ((char *)stack_begin) + stack_size;
  void **ptr = (void **)stack_end;
  /* ptr is the pointer past the allocation */
  ptr--;
  *ptr = (void*)initial_pc;
  ptr--;
  *ptr = stack_end - 20; /* rbp */;
  ptr-=5;
  return ptr;
}






struct thread_t {
  int ready; // 1 si le thread est prêt, 0 sinon
  coroutine_t coroutine;
  void* start;  // addresse du début de la pile
};
//Underscore t pour définir que c'est un "type"

coroutine_t scheduling;
#define MAX_THREADS 10
struct thread_t globalThreads[MAX_THREADS];
int num_threads = 0;
int current_thread = -1;

void scheduler_fn() {
  while(1) {
    if (globalThreads[current_thread].ready==1) {
      //fprintf(stderr, "switching to thread %d\n", current_thread);
      //switch_coroutine(&scheduling, globalThreads[current_thread].coroutine);
      
      /* proposition : remplacer switch_coroutine par le code suivant */
   assert(mprotect(globalThreads[current_thread].start, STACK_SIZE, PROT_READ | PROT_WRITE)==0);
      switch_coroutine(&scheduling, globalThreads[current_thread].coroutine);
      assert(mprotect(globalThreads[current_thread].start, STACK_SIZE, PROT_NONE)==0);

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
  char* stack  = (char*)aligned_alloc(4096, STACK_SIZE);
  assert(stack);
  globalThreads[num_threads].coroutine = init_coroutine(stack, STACK_SIZE, f);
  assert(mprotect(stack, STACK_SIZE, PROT_NONE)==0);
  globalThreads[num_threads].ready = 1;
  globalThreads[num_threads].start = &stack[0];
  num_threads++;
  assert(num_threads <= MAX_THREADS);
}

char byte_read;
int byte_set = 0; /* 1 si byte_read n'a pas été consommé, 0 sinon */

/* producteur */
void task1(void) {
  while(1) {
    int user_input = getchar();
    if (user_input =='e') {
      struct thread_t thread_2 = globalThreads[1];
      void * stack_2 = thread_2.start;
      printf("reading %d\n",  *((int*)stack_2));
    }
    if (user_input != -1) {
      byte_read = (char)user_input;
      byte_set = 1;
    }
    yield();
  }
}

/* consommateur 1 */
void task2(void) {
  while(1) {
    if (byte_set) {
      putchar(byte_read);
      byte_set = 0;
    };
    yield();
  }
}

/* consommateur 2 */
void task3 () {
   task2();
}


void segfault_handler(int signum) {
  assert(signum == SIGSEGV);
  printf("Thread %d segfaulted\n", current_thread);
  abort();
}

int main() {
  // rendre getchar non bloquant
  fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
  
  scheduling = init_coroutine(stack3, STACK_SIZE, scheduler_fn);
 
  signal(SIGSEGV, segfault_handler);
  
  thread_create(task1);
  thread_create(task2);
  thread_create(task3);
  enter_coroutine(scheduling);
}

