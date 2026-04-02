#ifndef AXT_PROGRESS_H
#define AXT_PROGRESS_H

#include "ast.h"

#ifndef _WIN32
#include <pthread.h>
#endif

typedef struct {
  int total;
  int num_workers;
  TestCaseResult *results; /* shared results array */

  /* Progress counters - protected by mutex */
#ifndef _WIN32
  pthread_mutex_t mutex;
#endif
  int completed;
  int failed;

  /* Worker status - array of num_workers strings */
  char **worker_names;
#ifndef _WIN32
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
