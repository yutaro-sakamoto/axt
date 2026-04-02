#include "ast.h"
#include <stdio.h>
#include <string.h>

TestSuite *parsed_suite = NULL;
TestCase *current_case = NULL;
char *input_basedir = NULL;
char *input_basename = NULL;

TestSuite *suite_create(char *title) {
  TestSuite *s = calloc(1, sizeof(TestSuite));
  s->title = title;
  return s;
}

TestCase *testcase_create(char *description) {
  TestCase *tc = calloc(1, sizeof(TestCase));
  tc->description = description;
  return tc;
}

void suite_add_case(TestSuite *suite, TestCase *tc) {
  suite->num_cases++;
  tc->id = suite->num_cases;
  if (!suite->cases) {
    suite->cases = tc;
    suite->cases_tail = tc;
  } else {
    suite->cases_tail->next = tc;
    suite->cases_tail = tc;
  }
}

TestStep *step_check_create(char **args, int nargs) {
  TestStep *step = calloc(1, sizeof(TestStep));
  step->type = STEP_AT_CHECK;
  step->u.check.command = args[0];
  if (nargs >= 2) {
    step->u.check.exit_code = atoi(args[1]);
    step->u.check.has_exit = 1;
    free(args[1]);
  } else {
    step->u.check.exit_code = 0;
  }
  if (nargs >= 3) {
    step->u.check.expect_stdout = args[2];
    step->u.check.has_stdout = 1;
  } else {
    step->u.check.expect_stdout = strdup("");
  }
  if (nargs >= 4) {
    step->u.check.expect_stderr = args[3];
    step->u.check.has_stderr = 1;
  } else {
    step->u.check.expect_stderr = strdup("");
  }
  return step;
}

TestStep *step_data_create(char *filename, char *content) {
  TestStep *step = calloc(1, sizeof(TestStep));
  step->type = STEP_AT_DATA;
  step->u.data.filename = filename;
  step->u.data.content = content;
  return step;
}

void testcase_add_step(TestCase *tc, TestStep *step) {
  if (!tc->steps) {
    tc->steps = step;
    tc->steps_tail = step;
  } else {
    tc->steps_tail->next = step;
    tc->steps_tail = step;
  }
}

static void step_free(TestStep *step) {
  while (step) {
    TestStep *next = step->next;
    if (step->type == STEP_AT_CHECK) {
      free(step->u.check.command);
      free(step->u.check.expect_stdout);
      free(step->u.check.expect_stderr);
    } else {
      free(step->u.data.filename);
      free(step->u.data.content);
    }
    free(step);
    step = next;
  }
}

static void testcase_free(TestCase *tc) {
  while (tc) {
    TestCase *next = tc->next;
    free(tc->description);
    step_free(tc->steps);
    free(tc);
    tc = next;
  }
}

void suite_free(TestSuite *suite) {
  if (!suite)
    return;
  free(suite->title);
  testcase_free(suite->cases);
  free(suite);
}
