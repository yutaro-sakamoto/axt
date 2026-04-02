#include "runner.h"
#include "os_compat.h"
#include "varexpand.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static TestCaseResult make_result(TestResult r, const char *msg,
                                  const char *case_name) {
  TestCaseResult res;
  res.result = r;
  res.message = msg ? strdup(msg) : NULL;
  res.case_name = case_name ? strdup(case_name) : NULL;
  return res;
}

TestCaseResult run_test_case(const TestCase *tc, const char *workdir,
                             int verbose) {
  TestStep *step = tc->steps;

  while (step) {
    if (step->type == STEP_AT_DATA) {
      /* Create file in workdir */
      char filepath[4096];
      snprintf(filepath, sizeof(filepath), "%s/%s", workdir,
               step->u.data.filename);

      /* Variable expand on content */
      char *expanded = NULL;
      char *undef_var = NULL;
      if (varexpand(step->u.data.content, &expanded, &undef_var) < 0) {
        char msg[512];
        snprintf(msg, sizeof(msg), "undefined variable: %s (in AT_DATA)",
                 undef_var ? undef_var : "?");
        free(undef_var);
        return make_result(RESULT_VAREXPAND_ERROR, msg, tc->description);
      }

      /* Create parent directories if needed */
      char *last_slash = strrchr(filepath, '/');
      if (last_slash) {
        *last_slash = '\0';
        os_mkdir_p(filepath);
        *last_slash = '/';
      }

      if (os_write_file(filepath, expanded, strlen(expanded)) < 0) {
        free(expanded);
        char msg[512];
        snprintf(msg, sizeof(msg), "failed to write file: %s", filepath);
        return make_result(RESULT_FAIL, msg, tc->description);
      }
      free(expanded);

    } else if (step->type == STEP_AT_CHECK) {
      /* Variable expand on command */
      char *expanded_cmd = NULL;
      char *undef_var = NULL;
      if (varexpand(step->u.check.command, &expanded_cmd, &undef_var) < 0) {
        char msg[512];
        snprintf(msg, sizeof(msg), "undefined variable: %s (in AT_CHECK)",
                 undef_var ? undef_var : "?");
        free(undef_var);
        return make_result(RESULT_VAREXPAND_ERROR, msg, tc->description);
      }

      char *actual_stdout = NULL;
      char *actual_stderr = NULL;
      int actual_exit = -1;

      if (verbose) {
        fprintf(stderr, "  AT_CHECK: %s\n", expanded_cmd);
      }

      if (os_exec_capture(expanded_cmd, &actual_stdout, &actual_stderr,
                          &actual_exit, workdir) < 0) {
        free(expanded_cmd);
        return make_result(RESULT_FAIL, "failed to execute command",
                           tc->description);
      }
      free(expanded_cmd);

      /* Check exit code */
      if (actual_exit != step->u.check.exit_code) {
        char msg[1024];
        snprintf(msg, sizeof(msg),
                 "exit code mismatch: expected %d, got %d\n"
                 "--- command: %s\n"
                 "--- stdout: %s\n"
                 "--- stderr: %s",
                 step->u.check.exit_code, actual_exit, step->u.check.command,
                 actual_stdout ? actual_stdout : "",
                 actual_stderr ? actual_stderr : "");
        free(actual_stdout);
        free(actual_stderr);
        return make_result(RESULT_FAIL, msg, tc->description);
      }

      /* Check stdout */
      if (step->u.check.has_stdout || strlen(step->u.check.expect_stdout) > 0) {
        /* Variable expand on expected stdout */
        char *exp_stdout = NULL;
        if (varexpand(step->u.check.expect_stdout, &exp_stdout, NULL) < 0) {
          free(actual_stdout);
          free(actual_stderr);
          return make_result(RESULT_VAREXPAND_ERROR,
                             "variable expansion error in expected stdout",
                             tc->description);
        }
        if (strcmp(actual_stdout ? actual_stdout : "", exp_stdout) != 0) {
          char msg[2048];
          snprintf(msg, sizeof(msg),
                   "stdout mismatch:\n"
                   "--- expected ---\n%s\n"
                   "--- actual ---\n%s",
                   exp_stdout, actual_stdout ? actual_stdout : "");
          free(exp_stdout);
          free(actual_stdout);
          free(actual_stderr);
          return make_result(RESULT_FAIL, msg, tc->description);
        }
        free(exp_stdout);
      }

      /* Check stderr */
      if (step->u.check.has_stderr || strlen(step->u.check.expect_stderr) > 0) {
        char *exp_stderr = NULL;
        if (varexpand(step->u.check.expect_stderr, &exp_stderr, NULL) < 0) {
          free(actual_stdout);
          free(actual_stderr);
          return make_result(RESULT_VAREXPAND_ERROR,
                             "variable expansion error in expected stderr",
                             tc->description);
        }
        if (strcmp(actual_stderr ? actual_stderr : "", exp_stderr) != 0) {
          char msg[2048];
          snprintf(msg, sizeof(msg),
                   "stderr mismatch:\n"
                   "--- expected ---\n%s\n"
                   "--- actual ---\n%s",
                   exp_stderr, actual_stderr ? actual_stderr : "");
          free(exp_stderr);
          free(actual_stdout);
          free(actual_stderr);
          return make_result(RESULT_FAIL, msg, tc->description);
        }
        free(exp_stderr);
      }

      free(actual_stdout);
      free(actual_stderr);
    }

    step = step->next;
  }

  return make_result(RESULT_PASS, NULL, tc->description);
}
