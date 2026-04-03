#ifdef _WIN32

#include "os_compat.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <direct.h>
#include <windows.h>

#ifndef strdup
#define strdup _strdup
#endif

/* Read all data from a pipe handle into a dynamically allocated string */
static char *read_handle(HANDLE h) {
  size_t cap = 1024, len = 0;
  char *buf = malloc(cap);
  if (!buf)
    return strdup("");
  for (;;) {
    if (len + 256 >= cap) {
      cap *= 2;
      char *tmp = realloc(buf, cap);
      if (!tmp)
        break;
      buf = tmp;
    }
    DWORD n = 0;
    BOOL ok = ReadFile(h, buf + len, (DWORD)(cap - len - 1), &n, NULL);
    if (!ok || n == 0)
      break;
    len += n;
  }
  buf[len] = '\0';
  return buf;
}

int os_exec_capture(const char *command, char **out_stdout, char **out_stderr,
                    int *exit_code, const char *workdir) {
  SECURITY_ATTRIBUTES sa;
  sa.nLength = sizeof(sa);
  sa.bInheritHandle = TRUE;
  sa.lpSecurityDescriptor = NULL;

  HANDLE stdout_rd = NULL, stdout_wr = NULL;
  HANDLE stderr_rd = NULL, stderr_wr = NULL;

  if (!CreatePipe(&stdout_rd, &stdout_wr, &sa, 0))
    return -1;
  SetHandleInformation(stdout_rd, HANDLE_FLAG_INHERIT, 0);

  if (!CreatePipe(&stderr_rd, &stderr_wr, &sa, 0)) {
    CloseHandle(stdout_rd);
    CloseHandle(stdout_wr);
    return -1;
  }
  SetHandleInformation(stderr_rd, HANDLE_FLAG_INHERIT, 0);

  STARTUPINFOA si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  si.hStdOutput = stdout_wr;
  si.hStdError = stderr_wr;
  si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
  si.dwFlags |= STARTF_USESTDHANDLES;
  ZeroMemory(&pi, sizeof(pi));

  /* Write command to a temp script file, then execute with sh.
   * This avoids all command-line quoting issues on Windows. */
  char tmppath[MAX_PATH];
  char tmpfile[MAX_PATH];
  GetTempPathA(MAX_PATH, tmppath);
  GetTempFileNameA(tmppath, "axt", 0, tmpfile);

  FILE *fp = fopen(tmpfile, "w");
  if (!fp) {
    CloseHandle(stdout_rd);
    CloseHandle(stdout_wr);
    CloseHandle(stderr_rd);
    CloseHandle(stderr_wr);
    return -1;
  }
  fputs(command, fp);
  fputs("\n", fp);
  fclose(fp);

  size_t cmdlen = strlen(tmpfile) + 8;
  char *cmdline = malloc(cmdlen);
  snprintf(cmdline, cmdlen, "sh \"%s\"", tmpfile);

  BOOL created = CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, 0, NULL,
                                workdir, &si, &pi);
  free(cmdline);

  if (!created) {
    DeleteFileA(tmpfile);
    CloseHandle(stdout_rd);
    CloseHandle(stdout_wr);
    CloseHandle(stderr_rd);
    CloseHandle(stderr_wr);
    return -1;
  }

  /* Close write ends so reads can detect EOF */
  CloseHandle(stdout_wr);
  CloseHandle(stderr_wr);

  *out_stdout = read_handle(stdout_rd);
  *out_stderr = read_handle(stderr_rd);
  CloseHandle(stdout_rd);
  CloseHandle(stderr_rd);

  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD ec = 0;
  GetExitCodeProcess(pi.hProcess, &ec);
  *exit_code = (int)ec;

  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  DeleteFileA(tmpfile);

  return 0;
}

int os_mkdir_p(const char *path) {
  char tmp[4096];
  snprintf(tmp, sizeof(tmp), "%s", path);
  size_t len = strlen(tmp);

  /* Remove trailing slash/backslash */
  if (len > 0 && (tmp[len - 1] == '/' || tmp[len - 1] == '\\'))
    tmp[--len] = '\0';

  for (char *p = tmp + 1; *p; p++) {
    if (*p == '/' || *p == '\\') {
      char saved = *p;
      *p = '\0';
      _mkdir(tmp); /* ignore errors for intermediate dirs */
      *p = saved;
    }
  }
  if (_mkdir(tmp) < 0 && errno != EEXIST)
    return -1;
  return 0;
}

int os_rmdir_r(const char *path) {
  char pattern[4096];
  snprintf(pattern, sizeof(pattern), "%s\\*", path);

  WIN32_FIND_DATAA fd;
  HANDLE hFind = FindFirstFileA(pattern, &fd);
  if (hFind == INVALID_HANDLE_VALUE)
    return -1;

  do {
    if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
      continue;
    char fullpath[4096];
    snprintf(fullpath, sizeof(fullpath), "%s\\%s", path, fd.cFileName);
    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      os_rmdir_r(fullpath);
    } else {
      DeleteFileA(fullpath);
    }
  } while (FindNextFileA(hFind, &fd));

  FindClose(hFind);
  return RemoveDirectoryA(path) ? 0 : -1;
}

int os_write_file(const char *path, const char *content, size_t len) {
  FILE *f = fopen(path, "wb");
  if (!f)
    return -1;
  if (len > 0)
    fwrite(content, 1, len, f);
  fclose(f);
  return 0;
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
  for (;;) {
    /* Ensure there is room to read */
    if (len + 256 >= cap) {
      cap *= 2;
      char *tmp = realloc(buf, cap);
      if (!tmp)
        break;
      buf = tmp;
    }
    ssize_t n = read(fd, buf + len, cap - len - 1);
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

  const struct dirent *ent;
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
