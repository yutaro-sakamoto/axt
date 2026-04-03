#include "progress.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
/* strdup is available as _strdup on MSVC */
#ifndef strdup
#define strdup _strdup
#endif
#else
#include <unistd.h>
#endif

#define BAR_WIDTH 40

void progress_init(ProgressCtx *ctx, int total, int num_workers,
                   TestCaseResult *results) {
  ctx->total = total;
  ctx->num_workers = num_workers;
  ctx->results = results;
  ctx->completed = 0;
  ctx->failed = 0;
  ctx->done = 0;

  ctx->worker_names = calloc((size_t)num_workers, sizeof(char *));
  for (int i = 0; i < num_workers; i++) {
    ctx->worker_names[i] = strdup("(idle)");
  }

#ifdef _WIN32
  InitializeCriticalSection(&ctx->mutex);
  InitializeCriticalSection(&ctx->worker_mutex);
#else
  pthread_mutex_init(&ctx->mutex, NULL);
  pthread_mutex_init(&ctx->worker_mutex, NULL);
#endif
}

void progress_complete(ProgressCtx *ctx, int passed) {
#ifdef _WIN32
  EnterCriticalSection(&ctx->mutex);
#else
  pthread_mutex_lock(&ctx->mutex);
#endif
  ctx->completed++;
  if (!passed)
    ctx->failed++;
#ifdef _WIN32
  LeaveCriticalSection(&ctx->mutex);
#else
  pthread_mutex_unlock(&ctx->mutex);
#endif
}

void progress_set_worker(ProgressCtx *ctx, int worker_id, const char *name) {
  if (worker_id < 0 || worker_id >= ctx->num_workers)
    return;
#ifdef _WIN32
  EnterCriticalSection(&ctx->worker_mutex);
#else
  pthread_mutex_lock(&ctx->worker_mutex);
#endif
  free(ctx->worker_names[worker_id]);
  ctx->worker_names[worker_id] = strdup(name ? name : "(idle)");
#ifdef _WIN32
  LeaveCriticalSection(&ctx->worker_mutex);
#else
  pthread_mutex_unlock(&ctx->worker_mutex);
#endif
}

static void render_progress(ProgressCtx *ctx) {
  int total = ctx->total;
  int num_workers = ctx->num_workers;

#ifdef _WIN32
  EnterCriticalSection(&ctx->mutex);
#else
  pthread_mutex_lock(&ctx->mutex);
#endif
  int completed = ctx->completed;
  int failed = ctx->failed;
#ifdef _WIN32
  LeaveCriticalSection(&ctx->mutex);
#else
  pthread_mutex_unlock(&ctx->mutex);
#endif

  /* Build the progress bar proportionally */
  char bar[BAR_WIDTH + 1];
  int passed = completed - failed;
  /* Proportional bar: passed=#, failed=x, pending=. */
  int pass_chars = (total > 0) ? (passed * BAR_WIDTH / total) : 0;
  int fail_chars = (total > 0) ? (failed * BAR_WIDTH / total) : 0;
  /* Ensure at least 1 char for failed if there are failures */
  if (failed > 0 && fail_chars == 0)
    fail_chars = 1;
  /* Adjust pass_chars if total exceeds bar width */
  if (pass_chars + fail_chars > BAR_WIDTH)
    pass_chars = BAR_WIDTH - fail_chars;
  int pending_chars = BAR_WIDTH - pass_chars - fail_chars;

  int pos = 0;
  for (int i = 0; i < pass_chars; i++)
    bar[pos++] = '#';
  for (int i = 0; i < fail_chars; i++)
    bar[pos++] = 'x';
  for (int i = 0; i < pending_chars; i++)
    bar[pos++] = '.';
  bar[BAR_WIDTH] = '\0';

  /* Move cursor up and clear lines (num_workers + 2 lines: bar + progress +
   * workers) */
  static int first_render = 1;
  if (!first_render) {
    /* Move cursor up by (2 + num_workers) lines */
    fprintf(stderr, "\033[%dA", 2 + num_workers);
  }
  first_render = 0;

  /* Render bar */
  fprintf(stderr, "\033[K[%s]\n", bar);
  fprintf(stderr, "\033[KProgress: %d/%d completed, %d failed\n", completed,
          total, failed);

  /* Render worker status */
#ifdef _WIN32
  EnterCriticalSection(&ctx->worker_mutex);
#else
  pthread_mutex_lock(&ctx->worker_mutex);
#endif
  for (int i = 0; i < num_workers; i++) {
    fprintf(stderr, "\033[KWorker %d: %s\n", i + 1, ctx->worker_names[i]);
  }
#ifdef _WIN32
  LeaveCriticalSection(&ctx->worker_mutex);
#else
  pthread_mutex_unlock(&ctx->worker_mutex);
#endif

  fflush(stderr);
}

void *progress_thread_func(void *arg) {
  ProgressCtx *ctx = (ProgressCtx *)arg;

  while (!ctx->done) {
    render_progress(ctx);
#ifdef _WIN32
    Sleep(100); /* 100ms */
#else
    usleep(100000); /* 100ms */
#endif
  }

  /* Final render */
  render_progress(ctx);

  /* Clear progress lines after completion */
  int lines = 2 + ctx->num_workers;
  fprintf(stderr, "\033[%dA", lines);
  for (int i = 0; i < lines; i++) {
    fprintf(stderr, "\033[K\n");
  }
  fprintf(stderr, "\033[%dA", lines);
  fflush(stderr);

  return NULL;
}

void progress_cleanup(ProgressCtx *ctx) {
  for (int i = 0; i < ctx->num_workers; i++) {
    free(ctx->worker_names[i]);
  }
  free(ctx->worker_names);
#ifdef _WIN32
  DeleteCriticalSection(&ctx->mutex);
  DeleteCriticalSection(&ctx->worker_mutex);
#else
  pthread_mutex_destroy(&ctx->mutex);
  pthread_mutex_destroy(&ctx->worker_mutex);
#endif
}
