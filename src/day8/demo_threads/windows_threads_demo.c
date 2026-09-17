#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>

enum { ITERATIONS_PER_THREAD = 2000000, THREAD_COUNT = 2 };

typedef struct {
    int worker_id;
    LONG *shared_counter;
    CRITICAL_SECTION *counter_mutex;
} worker_arguments_t;

static DWORD WINAPI worker(LPVOID raw_arguments)
{
    worker_arguments_t *arguments = raw_arguments;
    printf("WORKER %d START\n", arguments->worker_id);

    for (int i = 0; i < ITERATIONS_PER_THREAD; ++i) {
        EnterCriticalSection(arguments->counter_mutex);
        ++*arguments->shared_counter;
        LeaveCriticalSection(arguments->counter_mutex);
        if (i % 100000 == 0) {
            printf("Worker: %i, iteration: %i\n", arguments->worker_id, i);
        }
    }

    printf("WORKER %d DONE\n", arguments->worker_id);
    return 0;
}

int main(void)
{
    HANDLE threads[THREAD_COUNT];
    worker_arguments_t arguments[THREAD_COUNT];
    CRITICAL_SECTION counter_mutex;
    LONG shared_counter = 0;
    InitializeCriticalSection(&counter_mutex);

    for (int i = 0; i < THREAD_COUNT; ++i) {
        arguments[i] = (worker_arguments_t){i + 1, &shared_counter, &counter_mutex};
        threads[i] = CreateThread(NULL, 0, worker, &arguments[i], 0, NULL);
        if (threads[i] == NULL) {
            fprintf(stderr, "CreateThread failed: %lu\n", GetLastError());
            return EXIT_FAILURE;
        }
    }

    DWORD wait_result = WaitForMultipleObjects(THREAD_COUNT, threads, TRUE, INFINITE);
    if (wait_result == WAIT_FAILED) {
        fprintf(stderr, "WaitForMultipleObjects failed: %lu\n", GetLastError());
        return EXIT_FAILURE;
    }

    for (int i = 0; i < THREAD_COUNT; ++i) {
        CloseHandle(threads[i]);
    }
    DeleteCriticalSection(&counter_mutex);

    const LONG expected = THREAD_COUNT * ITERATIONS_PER_THREAD;
    printf("RESULT counter=%ld expected=%ld\n", shared_counter, expected);
    return shared_counter == expected ? EXIT_SUCCESS : EXIT_FAILURE;
}
