#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

int get_token(char **ps, const char *es, char **q, char **eq);
char *copy_token(char *s, const char *es);
int peek(char **ps, const char *es, char *toks);

