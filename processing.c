#include "processing.h"

static char whitespace[] = " \t\r\n\v";
static char symbols[] = "<|>(;)";

int get_token(char **ps, const char *es, char **q, char **eq)
{
	char *s;
	int ret;

	s = *ps;
	while(s < es && strchr(whitespace, *s))
		s++;
	if(q)
		*q = s;
	ret = *s;
	switch(*s){
	case 0:
		break;
	case '|':
	case '<':
	case '>':
	case '(':
	case ';':
	case ')':
	case '&': // this symbol should appear only in the very end!
		s++;
		break;
	default:
		ret = 'a'; //means we work with a command
		while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
			s++;
		break;
	}
	if(eq)
		*eq = s;

	while(s < es && strchr(whitespace, *s))
		s++;
	*ps = s;
	return ret;
}

int peek(char **ps, const char *es, char *toks)
{
	char *s = *ps;
	while(s < es && strchr(whitespace, *s))
		s++;
	*ps = s;
	return *s && strchr(toks, *s);
}

// make a copy of the characters in the input buffer, starting from s through es.
// null-terminate the copy to make it a string.
char *copy_token(char *s, const char *es)
{
	int diff = (int)(es - s);
	char *str = malloc(diff+1);
	if (!str) {
		fprintf(stderr, "Error in allocation\n");
		exit(-1);
	}
	strncpy(str, s, diff);
	str[diff] = 0;
	return str;
}

int detect_background(const char *buf)
{
	assert(buf);
	int is_background = 0;
	const char *s = buf + strlen(buf) - 1; // s points to one element before the final '\0'
	while (s != buf) {
		if(strchr(whitespace, *s))
			s--;
		else
			break;
	}
	if (peek((char **)&s, buf + strlen(buf), "&"))
		is_background = 1;
	return is_background;
}