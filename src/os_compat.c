#ifdef _WIN32
/* Windows stubs - to be implemented */
#include "os_compat.h"
#include <stdio.h>

int os_exec_capture(const char *command, char **out_stdout, char **out_stderr,
                    int *exit_code, const char *workdir) {
  (void)command;
  (void)out_stdout;
  (void)out_stderr;
  (void)exit_code;
  (void)workdir;
  fprintf(stderr, "error: Windows support not yet implemented\n");
  return -1;
}
int os_mkdir_p(const char *path) {
  (void)path;
  return -1;
}
int os_rmdir_r(const char *path) {
  (void)path;
  return -1;
}
int os_write_file(const char *path, const char *content, size_t len) {
  (void)path;
  (void)content;
  (void)len;
  return -1;
}

#else /* Unix */

#include "os_compat.h"
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* Read all data from fd into a dynamically allocated string */
static char *read_fd(int fd) {
  size_t cap = 1024, len = 0;
  char *buf = malloc(cap);
  if (!buf)
    return strdup("");
  ssize_t n;
  for (;;) {
    /* Ensure there is room to read */
    if (len + 256 >= cap) {
      cap *= 2;
      char *tmp = realloc(buf, cap);
      if (!tmp)
        break;
      buf = tmp;
    }
    n = read(fd, buf + len, cap - len - 1);
    if (n <= 0)
      break;
    len += (size_t)n;
  }
  buf[len] = '\0';
  return buf;
}

int os_exec_capture(const char *command, char **out_stdout, char **out_stderr,
                    int *exit_code, const char *workdir) {
  int stdout_pipe[2], stderr_pipe[2];

  if (pipe(stdout_pipe) < 0)
    return -1;
  if (pipe(stderr_pipe) < 0) {
    close(stdout_pipe[0]);
    close(stdout_pipe[1]);
    return -1;
  }

  pid_t pid = fork();
  if (pid < 0) {
    close(stdout_pipe[0]);
    close(stdout_pipe[1]);
    close(stderr_pipe[0]);
    close(stderr_pipe[1]);
    return -1;
  }

  if (pid == 0) {
    /* Child */
    close(stdout_pipe[0]);
    close(stderr_pipe[0]);
    dup2(stdout_pipe[1], STDOUT_FILENO);
    dup2(stderr_pipe[1], STDERR_FILENO);
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    if (workdir && chdir(workdir) < 0) {
      _exit(127);
    }

    execl("/bin/sh", "sh", "-c", command, (char *)NULL);
    _exit(127);
  }

  /* Parent */
  close(stdout_pipe[1]);
  close(stderr_pipe[1]);

  *out_stdout = read_fd(stdout_pipe[0]);
  *out_stderr = read_fd(stderr_pipe[0]);
  close(stdout_pipe[0]);
  close(stderr_pipe[0]);

  int status;
  while (waitpid(pid, &status, 0) < 0 && errno == EINTR)
    ;
  if (WIFEXITED(status)) {
    *exit_code = WEXITSTATUS(status);
  } else {
    *exit_code = -1;
  }

  return 0;
}

int os_mkdir_p(const char *path) {
  char tmp[4096];
  snprintf(tmp, sizeof(tmp), "%s", path);
  size_t len = strlen(tmp);

  /* Remove trailing slash */
  if (len > 0 && tmp[len - 1] == '/')
    tmp[--len] = '\0';

  for (char *p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = '\0';
      if (mkdir(tmp, 0755) < 0 && errno != EEXIST)
        return -1;
      *p = '/';
    }
  }
  if (mkdir(tmp, 0755) < 0 && errno != EEXIST)
    return -1;
  return 0;
}

int os_rmdir_r(const char *path) {
  DIR *d = opendir(path);
  if (!d)
    return -1;

  struct dirent *ent;
  char fullpath[4096];
  while ((ent = readdir(d)) != NULL) {
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
      continue;
    snprintf(fullpath, sizeof(fullpath), "%s/%s", path, ent->d_name);

    struct stat st;
    if (stat(fullpath, &st) < 0)
      continue;
    if (S_ISDIR(st.st_mode)) {
      os_rmdir_r(fullpath);
    } else {
      unlink(fullpath);
    }
  }
  closedir(d);
  return rmdir(path);
}

int os_write_file(const char *path, const char *content, size_t len) {
  FILE *f = fopen(path, "w");
  if (!f)
    return -1;
  if (len > 0)
    fwrite(content, 1, len, f);
  fclose(f);
  return 0;
}

#endif /* _WIN32 */
