#ifndef AXT_PROGRESS_H
#define AXT_PROGRESS_H

#include "ast.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <pthread.h>
#endif

typedef struct {
  int total;
  int num_workers;
  TestCaseResult *results; /* shared results array */

  /* Progress counters - protected by mutex */
#ifdef _WIN32
  CRITICAL_SECTION mutex;
#else
  pthread_mutex_t mutex;
#endif
  int completed;
  int failed;

  /* Worker status - array of num_workers strings */
  char **worker_names;
#ifdef _WIN32
  CRITICAL_SECTION worker_mutex;
#else
  pthread_mutex_t worker_mutex;
#endif

  /* Control */
  volatile int done;
} ProgressCtx;

/*
 * Initialize progress context.
 */
void progress_init(ProgressCtx *ctx, int total, int num_workers,
                   TestCaseResult *results);

/*
 * Record a test completion (called by worker threads).
 */
void progress_complete(ProgressCtx *ctx, int passed);

/*
 * Set worker status (called by worker threads).
 */
void progress_set_worker(ProgressCtx *ctx, int worker_id, const char *name);

/*
 * Progress rendering thread function.
 * Renders progress bar to stderr until ctx->done is set.
 */
void *progress_thread_func(void *arg);

/*
 * Clean up progress context.
 */
void progress_cleanup(ProgressCtx *ctx);

#endif /* AXT_PROGRESS_H */
