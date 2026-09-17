#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

enum { ITERATIONS_PER_THREAD = 2000000, THREAD_COUNT = 2 };

typedef struct {
    int worker_id;
    long *shared_counter;
    pthread_mutex_t *counter_mutex;
} worker_arguments_t;

static void *worker(void *raw_arguments)
{
    worker_arguments_t *arguments = raw_arguments;
    printf("WORKER %d START\n", arguments->worker_id);

    for (int i = 0; i < ITERATIONS_PER_THREAD; ++i) {
        int result = pthread_mutex_lock(arguments->counter_mutex);
        if (result != 0) {
            return (void *)1;
        }
        ++*arguments->shared_counter;
        pthread_mutex_unlock(arguments->counter_mutex);
        if (i % 100000 == 0) {
            printf("Worker: %i, iteration: %i\n", arguments->worker_id, i);
        }
    }

    printf("WORKER %d DONE\n", arguments->worker_id);
    return NULL;
}

int main(void)
{
    pthread_t threads[THREAD_COUNT];
    pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;
    long shared_counter = 0;
    worker_arguments_t arguments[THREAD_COUNT];

    for (int i = 0; i < THREAD_COUNT; ++i) {
        arguments[i] = (worker_arguments_t){
            .worker_id = i + 1,
            .shared_counter = &shared_counter,
            .counter_mutex = &counter_mutex,
        };
        int result = pthread_create(&threads[i], NULL, worker, &arguments[i]);
        if (result != 0) {
            fprintf(stderr, "pthread_create failed: %d\n", result);
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < THREAD_COUNT; ++i) {
        void *thread_result = NULL;
        int result = pthread_join(threads[i], &thread_result);
        if (result != 0 || thread_result != NULL) {
            fprintf(stderr, "worker %d failed\n", i + 1);
            return EXIT_FAILURE;
        }
    }

    pthread_mutex_destroy(&counter_mutex);
    const long expected = (long)THREAD_COUNT * ITERATIONS_PER_THREAD;
    printf("RESULT counter=%ld expected=%ld\n", shared_counter, expected);
    return shared_counter == expected ? EXIT_SUCCESS : EXIT_FAILURE;
}
