#ifndef AXT_VAREXPAND_H
#define AXT_VAREXPAND_H

/*
 * Expand $VAR and ${VAR} in input string.
 * Returns 0 on success, -1 if an undefined variable is encountered.
 * On success, *out is set to a newly allocated expanded string (caller frees).
 * On error, *undefined_var is set to the variable name (caller frees).
 */
int varexpand(const char *input, char **out, char **undefined_var);

#endif /* AXT_VAREXPAND_H */
