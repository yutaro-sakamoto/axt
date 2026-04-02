#ifndef AXT_THREADPOOL_H
#define AXT_THREADPOOL_H

typedef struct ThreadPool ThreadPool;
typedef void (*task_func_t)(void *arg);

/*
 * Create a thread pool with the given number of worker threads.
 */
ThreadPool *threadpool_create(int num_workers);

/*
 * Submit a task to the thread pool.
 */
void threadpool_submit(ThreadPool *pool, task_func_t func, void *arg);

/*
 * Wait for all submitted tasks to complete.
 */
void threadpool_wait(ThreadPool *pool);

/*
 * Destroy the thread pool (waits for tasks to complete first).
 */
void threadpool_destroy(ThreadPool *pool);

#endif /* AXT_THREADPOOL_H */
