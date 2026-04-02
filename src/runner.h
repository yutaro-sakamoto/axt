#ifndef AXT_RUNNER_H
#define AXT_RUNNER_H

#include "ast.h"

/*
 * Run a single test case in the given working directory.
 * The workdir is created before calling this function.
 * Returns the result (PASS, FAIL, or VAREXPAND_ERROR).
 */
TestCaseResult run_test_case(const TestCase *tc, const char *workdir,
                             int verbose);

#endif /* AXT_RUNNER_H */
