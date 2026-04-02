#ifndef AXT_ENVFILE_H
#define AXT_ENVFILE_H

/*
 * Load environment variables from a key=value file.
 * Lines starting with # are comments. Blank lines are skipped.
 * Returns 0 on success, -1 on error.
 */
int envfile_load(const char *path);

#endif /* AXT_ENVFILE_H */
