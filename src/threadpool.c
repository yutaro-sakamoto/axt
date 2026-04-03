#ifdef _WIN32

#include "threadpool.h"
#include <stdlib.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef struct Task {
  task_func_t func;
  void *arg;
  struct Task *next;
} Task;

struct ThreadPool {
  HANDLE *threads;
  int num_workers;

  /* Task queue */
  Task *queue_head;
  Task *queue_tail;
  CRITICAL_SECTION queue_cs;
  CONDITION_VARIABLE queue_cv;

  /* Completion tracking */
  int tasks_submitted;
  int tasks_completed;
  CRITICAL_SECTION done_cs;
  CONDITION_VARIABLE done_cv;

  /* Shutdown flag */
  int shutdown;
};

static DWORD WINAPI worker_func(LPVOID arg) {
  ThreadPool *pool = (ThreadPool *)arg;

  for (;;) {
    EnterCriticalSection(&pool->queue_cs);

    while (!pool->queue_head && !pool->shutdown) {
      SleepConditionVariableCS(&pool->queue_cv, &pool->queue_cs, INFINITE);
    }

    if (pool->shutdown && !pool->queue_head) {
      LeaveCriticalSection(&pool->queue_cs);
      break;
    }

    /* Dequeue task */
    Task *task = pool->queue_head;
    pool->queue_head = task->next;
    if (!pool->queue_head)
      pool->queue_tail = NULL;

    LeaveCriticalSection(&pool->queue_cs);

    /* Execute task */
    task->func(task->arg);
    free(task);

    /* Signal completion */
    EnterCriticalSection(&pool->done_cs);
    pool->tasks_completed++;
    WakeConditionVariable(&pool->done_cv);
    LeaveCriticalSection(&pool->done_cs);
  }

  return 0;
}

ThreadPool *threadpool_create(int num_workers) {
  ThreadPool *pool = calloc(1, sizeof(ThreadPool));
  pool->num_workers = num_workers;
  pool->threads = calloc((size_t)num_workers, sizeof(HANDLE));

  InitializeCriticalSection(&pool->queue_cs);
  InitializeConditionVariable(&pool->queue_cv);
  InitializeCriticalSection(&pool->done_cs);
  InitializeConditionVariable(&pool->done_cv);

  for (int i = 0; i < num_workers; i++) {
    pool->threads[i] = CreateThread(NULL, 0, worker_func, pool, 0, NULL);
  }

  return pool;
}

void threadpool_submit(ThreadPool *pool, task_func_t func, void *arg) {
  Task *task = calloc(1, sizeof(Task));
  task->func = func;
  task->arg = arg;

  EnterCriticalSection(&pool->done_cs);
  pool->tasks_submitted++;
  LeaveCriticalSection(&pool->done_cs);

  EnterCriticalSection(&pool->queue_cs);
  if (pool->queue_tail) {
    pool->queue_tail->next = task;
    pool->queue_tail = task;
  } else {
    pool->queue_head = task;
    pool->queue_tail = task;
  }
  WakeConditionVariable(&pool->queue_cv);
  LeaveCriticalSection(&pool->queue_cs);
}

void threadpool_wait(ThreadPool *pool) {
  EnterCriticalSection(&pool->done_cs);
  while (pool->tasks_completed < pool->tasks_submitted) {
    SleepConditionVariableCS(&pool->done_cv, &pool->done_cs, INFINITE);
  }
  LeaveCriticalSection(&pool->done_cs);
}

void threadpool_destroy(ThreadPool *pool) {
  if (!pool)
    return;

  /* Signal shutdown */
  EnterCriticalSection(&pool->queue_cs);
  pool->shutdown = 1;
  WakeAllConditionVariable(&pool->queue_cv);
  LeaveCriticalSection(&pool->queue_cs);

  /* Wait for all threads */
  WaitForMultipleObjects((DWORD)pool->num_workers, pool->threads, TRUE,
                         INFINITE);

  /* Cleanup */
  for (int i = 0; i < pool->num_workers; i++) {
    CloseHandle(pool->threads[i]);
  }
  DeleteCriticalSection(&pool->queue_cs);
  DeleteCriticalSection(&pool->done_cs);
  free(pool->threads);
  free(pool);
}

#else /* Unix / POSIX */

#include "threadpool.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

typedef struct Task {
  task_func_t func;
  void *arg;
  struct Task *next;
} Task;

struct ThreadPool {
  pthread_t *threads;
  int num_workers;

  /* Task queue */
  Task *queue_head;
  Task *queue_tail;
  pthread_mutex_t queue_mutex;
  pthread_cond_t queue_cond;

  /* Completion tracking */
  int tasks_submitted;
  int tasks_completed;
  pthread_mutex_t done_mutex;
  pthread_cond_t done_cond;

  /* Shutdown flag */
  int shutdown;
};

static void *worker_func(void *arg) {
  ThreadPool *pool = (ThreadPool *)arg;

  for (;;) {
    pthread_mutex_lock(&pool->queue_mutex);

    while (!pool->queue_head && !pool->shutdown) {
      pthread_cond_wait(&pool->queue_cond, &pool->queue_mutex);
    }

    if (pool->shutdown && !pool->queue_head) {
      pthread_mutex_unlock(&pool->queue_mutex);
      break;
    }

    /* Dequeue task */
    Task *task = pool->queue_head;
    pool->queue_head = task->next;
    if (!pool->queue_head)
      pool->queue_tail = NULL;

    pthread_mutex_unlock(&pool->queue_mutex);

    /* Execute task */
    task->func(task->arg);
    free(task);

    /* Signal completion */
    pthread_mutex_lock(&pool->done_mutex);
    pool->tasks_completed++;
    pthread_cond_signal(&pool->done_cond);
    pthread_mutex_unlock(&pool->done_mutex);
  }

  return NULL;
}

ThreadPool *threadpool_create(int num_workers) {
  ThreadPool *pool = calloc(1, sizeof(ThreadPool));
  pool->num_workers = num_workers;
  pool->threads = calloc((size_t)num_workers, sizeof(pthread_t));

  pthread_mutex_init(&pool->queue_mutex, NULL);
  pthread_cond_init(&pool->queue_cond, NULL);
  pthread_mutex_init(&pool->done_mutex, NULL);
  pthread_cond_init(&pool->done_cond, NULL);

  for (int i = 0; i < num_workers; i++) {
    pthread_create(&pool->threads[i], NULL, worker_func, pool);
  }

  return pool;
}

void threadpool_submit(ThreadPool *pool, task_func_t func, void *arg) {
  Task *task = calloc(1, sizeof(Task));
  task->func = func;
  task->arg = arg;

  pthread_mutex_lock(&pool->done_mutex);
  pool->tasks_submitted++;
  pthread_mutex_unlock(&pool->done_mutex);

  pthread_mutex_lock(&pool->queue_mutex);
  if (pool->queue_tail) {
    pool->queue_tail->next = task;
    pool->queue_tail = task;
  } else {
    pool->queue_head = task;
    pool->queue_tail = task;
  }
  pthread_cond_signal(&pool->queue_cond);
  pthread_mutex_unlock(&pool->queue_mutex);
}

void threadpool_wait(ThreadPool *pool) {
  pthread_mutex_lock(&pool->done_mutex);
  while (pool->tasks_completed < pool->tasks_submitted) {
    pthread_cond_wait(&pool->done_cond, &pool->done_mutex);
  }
  pthread_mutex_unlock(&pool->done_mutex);
}

void threadpool_destroy(ThreadPool *pool) {
  if (!pool)
    return;

  /* Signal shutdown */
  pthread_mutex_lock(&pool->queue_mutex);
  pool->shutdown = 1;
  pthread_cond_broadcast(&pool->queue_cond);
  pthread_mutex_unlock(&pool->queue_mutex);

  /* Join all threads */
  for (int i = 0; i < pool->num_workers; i++) {
    pthread_join(pool->threads[i], NULL);
  }

  /* Cleanup */
  pthread_mutex_destroy(&pool->queue_mutex);
  pthread_cond_destroy(&pool->queue_cond);
  pthread_mutex_destroy(&pool->done_mutex);
  pthread_cond_destroy(&pool->done_cond);
  free(pool->threads);
  free(pool);
}

#endif /* _WIN32 */
