#include "ast.h"
#include "envfile.h"
#include "os_compat.h"
#include "progress.h"
#include "runner.h"
#include "threadpool.h"
#include <stdio.h>
#include <string.h>

#ifndef _WIN32
#include <pthread.h>
#endif

extern FILE *yyin;
extern int yyparse(void);

#define AXT_VERSION "0.1.0"

typedef struct {
  int parallel;
  const char *envfile;
  int verbose;
  const char *input;
} Options;

static void print_usage(void) {
  fprintf(stderr, "usage: axt [options] <testfile.at>\n"
                  "\n"
                  "Options:\n"
                  "  -j <N>          Parallel execution count (default: 2)\n"
                  "  --env <file>    Environment variable file\n"
                  "  -v              Verbose output\n"
                  "  --help          Show help\n"
                  "  --version       Show version\n");
}

static int parse_options(int argc, const char *const argv[], Options *opts) {
  opts->parallel = 2;
  opts->envfile = NULL;
  opts->verbose = 0;
  opts->input = NULL;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--help") == 0) {
      print_usage();
      return -2;
    } else if (strcmp(argv[i], "--version") == 0) {
      printf("axt %s\n", AXT_VERSION);
      return -2;
    } else if (strcmp(argv[i], "-j") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "error: -j requires an argument\n");
        return -1;
      }
      opts->parallel = atoi(argv[++i]);
      if (opts->parallel < 1)
        opts->parallel = 1;
    } else if (strcmp(argv[i], "--env") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "error: --env requires an argument\n");
        return -1;
      }
      opts->envfile = argv[++i];
    } else if (strcmp(argv[i], "-v") == 0) {
      opts->verbose = 1;
    } else if (argv[i][0] == '-') {
      fprintf(stderr, "error: unknown option '%s'\n", argv[i]);
      return -1;
    } else {
      if (opts->input) {
        fprintf(stderr, "error: multiple input files not supported\n");
        return -1;
      }
      opts->input = argv[i];
    }
  }

  if (!opts->input) {
    fprintf(stderr, "error: no input file specified\n");
    print_usage();
    return -1;
  }

  return 0;
}

static void parse_input_path(const char *path) {
  const char *slash = strrchr(path, '/');
  if (slash) {
    size_t dirlen = (size_t)(slash - path);
    input_basedir = malloc(dirlen + 1);
    memcpy(input_basedir, path, dirlen);
    input_basedir[dirlen] = '\0';
    slash++;
  } else {
    input_basedir = strdup(".");
    slash = path;
  }

  const char *dot = strrchr(slash, '.');
  if (dot) {
    size_t len = (size_t)(dot - slash);
    input_basename = malloc(len + 1);
    memcpy(input_basename, slash, len);
    input_basename[len] = '\0';
  } else {
    input_basename = strdup(slash);
  }
}

static int pad_width(int n) {
  int w = 1;
  while (n >= 10) {
    n /= 10;
    w++;
  }
  return w;
}

/* Task argument for parallel execution */
typedef struct {
  const TestCase *tc;
  char workdir[4096];
  TestCaseResult *result;
  int verbose;
  ProgressCtx *progress;
  int worker_id;
} TaskArg;

/* Worker ID assignment */
#ifndef _WIN32
static pthread_mutex_t worker_id_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif
static int *worker_id_pool = NULL;
static int worker_id_pool_size = 0;

static int acquire_worker_id(void) {
  int id = -1;
#ifndef _WIN32
  pthread_mutex_lock(&worker_id_mutex);
#endif
  for (int i = 0; i < worker_id_pool_size; i++) {
    if (worker_id_pool[i] == 0) {
      worker_id_pool[i] = 1;
      id = i;
      break;
    }
  }
#ifndef _WIN32
  pthread_mutex_unlock(&worker_id_mutex);
#endif
  return id;
}

static void release_worker_id(int id) {
  if (id < 0)
    return;
#ifndef _WIN32
  pthread_mutex_lock(&worker_id_mutex);
#endif
  worker_id_pool[id] = 0;
#ifndef _WIN32
  pthread_mutex_unlock(&worker_id_mutex);
#endif
}

static void task_run(void *arg) {
  TaskArg *ta = (TaskArg *)arg;
  int wid = acquire_worker_id();

  if (ta->progress) {
    progress_set_worker(ta->progress, wid, ta->tc->description);
  }

  *ta->result = run_test_case(ta->tc, ta->workdir, ta->verbose);

  if (ta->progress) {
    progress_complete(ta->progress, ta->result->result == RESULT_PASS);
    progress_set_worker(ta->progress, wid, "(idle)");
  }

  release_worker_id(wid);
}

static int run_tests(TestSuite *suite, const char *basedir, int parallel,
                     int verbose) {
  int total = suite->num_cases;
  int width = pad_width(total);
  int id_width = pad_width(total);

  TestCaseResult *results = calloc((size_t)total, sizeof(TestCaseResult));
  TaskArg *tasks = calloc((size_t)total, sizeof(TaskArg));

  /* Initialize worker ID pool */
  worker_id_pool_size = parallel;
  worker_id_pool = calloc((size_t)parallel, sizeof(int));

  /* Setup progress context (only for parallel with multiple workers) */
  ProgressCtx progress;
  int use_progress = (parallel > 1 && total > 0);

  if (use_progress) {
    progress_init(&progress, total, parallel, results);
  }

  /* Prepare tasks */
  TestCase *tc = suite->cases;
  int idx = 0;
  while (tc) {
    tasks[idx].tc = tc;
    snprintf(tasks[idx].workdir, sizeof(tasks[idx].workdir), "%s/%0*d", basedir,
             width, tc->id);
    os_mkdir_p(tasks[idx].workdir);
    tasks[idx].result = &results[idx];
    tasks[idx].verbose = verbose;
    tasks[idx].progress = use_progress ? &progress : NULL;
    idx++;
    tc = tc->next;
  }

  if (parallel <= 1) {
    /* Sequential execution */
    for (int i = 0; i < total; i++) {
      if (verbose) {
        fprintf(stderr, "Running test %d: %s\n", tasks[i].tc->id,
                tasks[i].tc->description);
      }
      task_run(&tasks[i]);
    }
  } else {
    /* Parallel execution with progress bar */
#ifndef _WIN32
    pthread_t progress_thread;
    if (use_progress) {
      pthread_create(&progress_thread, NULL, progress_thread_func, &progress);
    }
#endif

    ThreadPool *pool = threadpool_create(parallel);
    for (int i = 0; i < total; i++) {
      threadpool_submit(pool, task_run, &tasks[i]);
    }
    threadpool_wait(pool);
    threadpool_destroy(pool);

#ifndef _WIN32
    if (use_progress) {
      progress.done = 1;
      pthread_join(progress_thread, NULL);
      progress_cleanup(&progress);
    }
#endif
  }

  /* Print results in order */
  int passed = 0, failed = 0;
  for (int i = 0; i < total; i++) {
    TestCaseResult *r = &results[i];
    const char *desc = tasks[i].tc->description;
    int id = tasks[i].tc->id;

    if (r->result == RESULT_PASS) {
      printf("%*d: %-40s ok\n", id_width, id, desc);
      passed++;
      os_rmdir_r(tasks[i].workdir);
    } else if (r->result == RESULT_VAREXPAND_ERROR) {
      printf("%*d: %-40s FAILED (variable expansion error)\n", id_width, id,
             desc);
      if (verbose && r->message) {
        fprintf(stderr, "  %s\n", r->message);
      }
      failed++;
    } else {
      printf("%*d: %-40s FAILED\n", id_width, id, desc);
      if (verbose && r->message) {
        fprintf(stderr, "  %s\n", r->message);
      }
      failed++;
    }

    free(r->message);
    free(r->case_name);
  }

  printf("\n---\n");
  printf("テスト数: %d  成功: %d  失敗: %d\n", total, passed, failed);

  free(results);
  free(tasks);
  free(worker_id_pool);
  worker_id_pool = NULL;
  return failed;
}

int main(int argc, char *argv[]) {
  Options opts;
  int rc = parse_options(argc, (const char *const *)argv, &opts);
  if (rc == -2)
    return 0;
  if (rc < 0)
    return 1;

  if (opts.envfile) {
    if (envfile_load(opts.envfile) < 0)
      return 1;
  }

  yyin = fopen(opts.input, "r");
  if (!yyin) {
    fprintf(stderr, "error: cannot open '%s'\n", opts.input);
    return 1;
  }

  parse_input_path(opts.input);

  int result = yyparse();
  fclose(yyin);

  if (result != 0 || !parsed_suite) {
    fprintf(stderr, "error: failed to parse '%s'\n", opts.input);
    free(input_basedir);
    free(input_basename);
    suite_free(parsed_suite);
    return 1;
  }

  char basedir[4096];
  if (input_basedir && input_basedir[0] && strcmp(input_basedir, ".") != 0) {
    snprintf(basedir, sizeof(basedir), "%s/%s.dir", input_basedir,
             input_basename);
  } else {
    snprintf(basedir, sizeof(basedir), "%s.dir", input_basename);
  }

  os_mkdir_p(basedir);

  printf("%s\n\n", parsed_suite->title);

  int failed = run_tests(parsed_suite, basedir, opts.parallel, opts.verbose);

  if (failed == 0) {
    os_rmdir_r(basedir);
  }

  suite_free(parsed_suite);
  free(input_basedir);
  free(input_basename);
  return failed > 0 ? 1 : 0;
}
