#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

/* Queue Structures */
typedef struct queue_node_s {
  struct queue_node_s *next;
  struct queue_node_s *prev;
  char c;
} queue_node_t;

typedef struct {
  struct queue_node_s *front;
  struct queue_node_s *back;
  pthread_mutex_t lock;
} queue_t;

/* Thread Function Prototypes */
void *producer_routine(void *arg);
void *consumer_routine(void *arg);

/* Global Data */
long g_num_prod; /* number of producer threads */
pthread_mutex_t g_num_prod_lock;

/* Main - entry point */
int main(int argc, char **argv) {
  queue_t queue;
  pthread_t producer_thread, consumer_thread;
  void *thread_return = NULL;
  int result = 0;

  /* Initialization */
  memset(&queue, 0, sizeof(queue));
  pthread_mutex_init(&queue.lock, NULL);
  pthread_mutex_init(&g_num_prod_lock, NULL); 
  g_num_prod = 1; /* there will be 1 producer thread */

  printf("Main thread started with thread id %lu\n", pthread_self());

  /* Create producer and consumer threads */
  result = pthread_create(&producer_thread, NULL, producer_routine, &queue);
  if (0 != result) {
    fprintf(stderr, "Failed to create producer thread: %s\n", strerror(result));
    exit(1);
  }

  printf("Producer thread started with thread id %lu\n", producer_thread);

  result = pthread_create(&consumer_thread, NULL, consumer_routine, &queue);
  if (0 != result) {
    fprintf(stderr, "Failed to create consumer thread: %s\n", strerror(result));
    exit(1);
  }

  /* Join threads, handle return values where appropriate */
  result = pthread_join(producer_thread, NULL);
  if (0 != result) {
    fprintf(stderr, "Failed to join producer thread: %s\n", strerror(result));
    pthread_exit(NULL);
  }

  result = pthread_join(consumer_thread, &thread_return);
  if (0 != result) {
    fprintf(stderr, "Failed to join consumer thread: %s\n", strerror(result));
    pthread_exit(NULL);
  }

  printf("\nPrinted %lu characters.\n", (long)thread_return);

  pthread_mutex_destroy(&queue.lock);
  pthread_mutex_destroy(&g_num_prod_lock);
  return 0;
}

/* producer_routine - thread that adds the letters 'a'-'z' to the queue */
void *producer_routine(void *arg) {
  queue_t *queue_p = arg;
  queue_node_t *new_node_p = NULL;
  pthread_t consumer_thread;
  void *thread_return = NULL;
  int result = 0;
  char c;

  result = pthread_create(&consumer_thread, NULL, consumer_routine, queue_p);
  if (0 != result) {
    fprintf(stderr, "Failed to create consumer thread: %s\n", strerror(result));
    exit(1);
  }

  for (c = 'a'; c <= 'z'; ++c) {
    /* Create a new node with the prev letter */
    new_node_p = malloc(sizeof(queue_node_t));
    new_node_p->c = c;
    new_node_p->next = NULL;

    /* Add the node to the queue */
    pthread_mutex_lock(&queue_p->lock);
    if (queue_p->back == NULL) {
      assert(queue_p->front == NULL);

      new_node_p->prev = NULL;
      queue_p->front = new_node_p;
      queue_p->back = new_node_p;
    }
    else {
      assert(queue_p->front != NULL);

      new_node_p->prev = queue_p->back;
      queue_p->back->next = new_node_p;
      queue_p->back = new_node_p;
    }

    pthread_mutex_unlock(&queue_p->lock);
	
    sched_yield();
  }

  /* Decrement the number of producer threads running, then return */
  pthread_mutex_lock(&g_num_prod_lock);
  --g_num_prod;
  pthread_mutex_unlock(&g_num_prod_lock);
  
  result = pthread_join(consumer_thread, &thread_return);
  if (0 != result) {
    fprintf(stderr, "Failed to join consumer thread: %s\n", strerror(result));
    pthread_exit(NULL);
  }

  printf("\nPrinted %lu characters.\n", (long)thread_return);

  return (void*) 0;
}

/* consumer_routine - thread that prints characters off the queue */
void *consumer_routine(void *arg) {
  queue_t *queue_p = arg;
  queue_node_t *prev_node_p = NULL;
  long count = 0; /* number of nodes this thread printed */

  printf("Consumer thread started with thread id %lu\n", pthread_self());

  /* terminate the loop only when there are no more items in the queue
   * AND the producer threads are all done */
  while(1) {
	int is_queue_locked = 0;
    pthread_mutex_lock(&g_num_prod_lock);
    if (g_num_prod <= 0) {
      pthread_mutex_unlock(&g_num_prod_lock);

      pthread_mutex_lock(&queue_p->lock);
      if (queue_p->front == NULL) {
        pthread_mutex_unlock(&queue_p->lock);
        break;
      }
      else {
		is_queue_locked = 1;
      }
    }
    else {
	  /* There is a producer but the production is yet to start, do something else */
      pthread_mutex_unlock(&g_num_prod_lock);

      sched_yield();
    }

	if (!is_queue_locked)
		pthread_mutex_lock(&queue_p->lock);

    if (queue_p->front != NULL) {
      /* Remove the prev item from the queue */
      prev_node_p = queue_p->front;

      if (queue_p->front->next == NULL)
        queue_p->back = NULL;
      else
        queue_p->front->next->prev = NULL;

      queue_p->front = queue_p->front->next;
      pthread_mutex_unlock(&queue_p->lock);

      /* Print the character, and increment the character count */
      printf("%c", prev_node_p->c);
      free(prev_node_p);
      ++count;
    }
    else {
      /* Queue is empty, so let some other thread run */
      pthread_mutex_unlock(&queue_p->lock);

      sched_yield();
    }
  }

  return (void*) count;
}
