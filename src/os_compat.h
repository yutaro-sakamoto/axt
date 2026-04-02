#ifndef AXT_OS_COMPAT_H
#define AXT_OS_COMPAT_H

#include <stddef.h>

/*
 * Execute a command via shell, capture stdout and stderr.
 * workdir: directory to run the command in (chdir before exec).
 * Returns 0 on success (exit_code is set), -1 on system error.
 */
int os_exec_capture(const char *command, char **out_stdout, char **out_stderr,
                    int *exit_code, const char *workdir);

/* Create directory and all parents. Returns 0 on success. */
int os_mkdir_p(const char *path);

/* Remove directory recursively. Returns 0 on success. */
int os_rmdir_r(const char *path);

/* Write content to a file. Returns 0 on success. */
int os_write_file(const char *path, const char *content, size_t len);

#endif /* AXT_OS_COMPAT_H */
