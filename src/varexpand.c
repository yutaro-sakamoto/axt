#include "varexpand.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_varchar(int c) { return isalnum(c) || c == '_'; }

int varexpand(const char *input, char **out, char **undefined_var) {
  size_t cap = 256, len = 0;
  char *buf = malloc(cap);
  const char *p = input;

  *out = NULL;
  if (undefined_var)
    *undefined_var = NULL;

#define APPEND(ch)                                                             \
  do {                                                                         \
    if (len + 1 >= cap) {                                                      \
      cap *= 2;                                                                \
      buf = realloc(buf, cap);                                                 \
    }                                                                          \
    buf[len++] = (ch);                                                         \
  } while (0)

  while (*p) {
    if (*p == '$') {
      p++;
      if (*p == '{') {
        /* ${VAR} form */
        p++;
        const char *start = p;
        while (*p && *p != '}')
          p++;
        if (*p != '}') {
          /* Malformed, treat as literal */
          APPEND('$');
          APPEND('{');
          p = start;
          continue;
        }
        size_t namelen = (size_t)(p - start);
        char *name = malloc(namelen + 1);
        memcpy(name, start, namelen);
        name[namelen] = '\0';
        p++; /* skip '}' */

        const char *val = getenv(name);
        if (!val) {
          if (undefined_var)
            *undefined_var = name;
          else
            free(name);
          free(buf);
          return -1;
        }
        free(name);
        while (*val)
          APPEND(*val++);
      } else if (is_varchar(*p)) {
        /* $VAR form */
        const char *start = p;
        while (*p && is_varchar(*p))
          p++;
        size_t namelen = (size_t)(p - start);
        char *name = malloc(namelen + 1);
        memcpy(name, start, namelen);
        name[namelen] = '\0';

        const char *val = getenv(name);
        if (!val) {
          if (undefined_var)
            *undefined_var = name;
          else
            free(name);
          free(buf);
          return -1;
        }
        free(name);
        while (*val)
          APPEND(*val++);
      } else {
        /* Lone $, treat as literal */
        APPEND('$');
      }
    } else {
      APPEND(*p);
      p++;
    }
  }

  buf[len] = '\0';
  *out = buf;
  return 0;

#undef APPEND
}
