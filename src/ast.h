#ifndef AXT_AST_H
#define AXT_AST_H

#include <stdlib.h>

/* Step types */
typedef enum {
  STEP_AT_CHECK,
  STEP_AT_DATA
} StepType;

typedef struct TestStep {
  StepType type;
  union {
    struct {
      char *command;
      int exit_code;
      char *expect_stdout;
      char *expect_stderr;
      int has_exit;
      int has_stdout;
      int has_stderr;
    } check;
    struct {
      char *filename;
      char *content;
    } data;
  } u;
  struct TestStep *next;
} TestStep;

typedef struct TestCase {
  int id;
  char *description;
  TestStep *steps;
  TestStep *steps_tail;
  struct TestCase *next;
} TestCase;

typedef struct TestSuite {
  char *title;
  TestCase *cases;
  TestCase *cases_tail;
  int num_cases;
} TestSuite;

/* Result types */
typedef enum {
  RESULT_PASS,
  RESULT_FAIL,
  RESULT_VAREXPAND_ERROR
} TestResult;

typedef struct TestCaseResult {
  TestResult result;
  char *message;
  char *case_name;
} TestCaseResult;

/* AST constructors */
TestSuite *suite_create(char *title);
TestCase *testcase_create(char *description);
void suite_add_case(TestSuite *suite, TestCase *tc);
TestStep *step_check_create(char **args, int nargs);
TestStep *step_data_create(char *filename, char *content);
void testcase_add_step(TestCase *tc, TestStep *step);
void suite_free(TestSuite *suite);

/* Parser globals */
extern TestSuite *parsed_suite;
extern TestCase *current_case;
extern char *input_basedir;
extern char *input_basename;

#endif /* AXT_AST_H */
