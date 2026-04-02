#include "envfile.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int envfile_load(const char *path) {
  FILE *f = fopen(path, "r");
  if (!f) {
    fprintf(stderr, "error: cannot open env file '%s'\n", path);
    return -1;
  }

  char line[4096];
  while (fgets(line, sizeof(line), f)) {
    /* Strip trailing newline */
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
      line[--len] = '\0';

    /* Skip blank lines and comments */
    const char *p = line;
    while (*p && isspace((unsigned char)*p))
      p++;
    if (*p == '\0' || *p == '#')
      continue;

    /* Find '=' separator */
    char *eq = strchr(line, '=');
    if (!eq)
      continue;

    *eq = '\0';
    const char *value = eq + 1;

    /* Trim leading and trailing whitespace from key */
    const char *key = line;
    while (*key && isspace((unsigned char)*key))
      key++;
    char *kend = eq - 1;
    while (kend >= key && isspace((unsigned char)*kend))
      *kend-- = '\0';

      /* Set env var only if not already set (user environment takes priority)
       */
#ifdef _WIN32
    {
      const char *existing = getenv(key);
      if (!existing) {
        _putenv_s(key, value);
      }
    }
#else
    setenv(key, value, 0);
#endif
  }

  fclose(f);
  return 0;
}
