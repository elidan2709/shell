#include "parser.h"

struct cmd* parsecmd(char *s)
{
	char *es;
	struct cmd *cmd;

	es = s + strlen(s);
	cmd = get_pipe(&s, es);
	if (peek(&s, es, "&"))
		get_token((char **)&s, es, 0, 0);
	if(s != es){
		fprintf(stderr, "leftovers: %s\n", s);
		exit(-1);
	}
	return cmd;
}

struct cmd* create_pipecmd(struct cmd *left, struct cmd *right)
{
	struct pipecmd *cmd = (struct pipecmd *)malloc(sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	cmd->type = '|';
	/*
	 * 1) cat f1 > f2 | grep sym -- prohibited
	 * 2) cat f1 | grep sym < f2 -- prohibited
	 * */
	if (left->type == '>' || left->type == '^' || right ->type == '<') {
		fprintf(stderr, "Bad usage of redirection and pipe opearations!\n");
		exit(-1);
	}
	cmd->left = left;
	cmd->right = right;
	return (struct cmd*)cmd;
}

struct cmd* create_execcmd(void)
{
	struct execcmd *cmd = (struct execcmd *)malloc(sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	cmd->type = ' ';
	cmd->next = NULL;
	return (struct cmd*)cmd;
}

struct cmd* create_redir(struct cmd *subcmd, char *file, int type)
{
	struct redircmd *cmd;

	cmd = malloc(sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	cmd->type = type;
	cmd->cmd = subcmd;
	cmd->file = file;
	switch(type) {
	case '<':
		cmd->mode = O_RDONLY;
		cmd->fd   = STDIN_FILENO;
		break;
	case '>':
		cmd->mode = O_WRONLY|O_CREAT|O_TRUNC;
		cmd->fd = STDOUT_FILENO;
		break;
	case '^':
		cmd->mode = O_WRONLY|O_CREAT|O_APPEND;
		cmd->fd = STDOUT_FILENO;
		break;
	default:
		fprintf(stderr, "abusive type of redirection operation: [%d]\n", type);
		exit(EXIT_FAILURE);
	}
	return (struct cmd*)cmd;
}


/*
 * Deal with pipe redirection return a pointer,
 * which contains two commands to execute in child processes
 * */
struct cmd* get_pipe(char **ps, char *es)
{
	struct cmd *left;
	left = get_parentheses(ps, es);
	if(peek(ps, es, "|")) {
		get_token(ps, es, 0, 0);
		left = create_pipecmd(left, get_pipe(ps, es)); // cmd1 | (cmd2; cmd3) -- is ok. But the meaning of such command is obscure
	}
	return left;
}

/*
 * In input this function takes exactly pointer to structure redircmd
 * we check it via macro and for returning value we have two major
 * cases, which described in the code below
 * */
struct cmd * cur_command(struct cmd *current)
{
	assert(current->type == '<' || current->type == '>' || current->type == '^');
	struct redircmd * redirected = (struct redircmd*)current;
	switch(redirected->cmd->type) {
		case '>':
		case '<':
		case '^':
			return (struct cmd *)((struct execcmd *)(((struct redircmd *)(((struct redircmd *)current)->cmd))->cmd));
		case ' ':
			return (struct cmd*)((struct execcmd *)(((struct redircmd *)current)->cmd));
		default:
			fprintf(stderr, "Ambiguous type for returning next command %d\n", redirected->cmd->type);
			exit(-1);
	}
}

struct cmd * change_redirection_in_sequence(char **ps, const char *es, struct cmd *cmd, int tok)
{
	//We'd like to return pure head of the linked list
	char *q, *eq;
	struct cmd *head = NULL; // the head of new list
	struct cmd *cur = cmd;
	int flag = 0;
	if(get_token(ps, es, &q, &eq) != 'a') {
		fprintf(stderr, "missing file for redirection\n");
		exit(-1);
	}
#define next(cur) ((struct execcmd *)(cur))->next
	do {
		if (cur->type == tok || cur->type == '|') {
			fprintf(stderr, "Error: redirection to file %s\n", *ps);
			exit(-1);
		}
		if (flag == 0) {
			head = create_redir(cur, copy_token(q, eq), tok); // (cat < file; ls) > src
			cur = cur_command(head);
			flag = 1;
		} else {
			if (tok == '>') // make the next write command in APPEND mode
				tok = '^';
			next(cur) = create_redir(next(cur), copy_token(q, eq), tok);
			cur = cur_command(next(cur));
		}
	}while(next(cur) != NULL);

#undef next
	assert(flag == 1);
	return head;
}


struct cmd* get_redirection(struct cmd *cmd, char **ps, char *es)
{
	int tok;
	while(peek(ps, es, "<>")) { // '>' '<' ">>"
		tok = get_token(ps, es, 0, 0);
		if (tok == '>' && peek(ps, es, ">")) {
			get_token(ps, es, 0, 0);
			tok = '^'; // this type means ">>"
		}
		cmd = change_redirection_in_sequence(ps, es, cmd, tok);
	}
	return cmd;
}

struct cmd *get_parentheses(char **ps, char *es)
{
	if(!peek(ps, es, "("))
		return get_exec(ps, es);
	struct cmd *cmd;
	get_token(ps, es, 0, 0);
	cmd = get_pipe(ps, es);
	if (!peek(ps, es, ")")) {
		fprintf(stderr, "no closing parenthesis: the rest part of a str: %s\n", *ps);
		exit(-1);
	}
	get_token(ps, es, 0, 0); // get ')'
	/*
	 * Here we should handle redirection applied after parentheses.
	 **/
	cmd = get_redirection(cmd, ps, es);

	return cmd;
}

struct cmd* get_exec(char **ps, char *es)
{
	char *q, *eq;
	int tok, argc;
	struct execcmd *cmd; // collect the sequence of commands in linked list
	struct cmd *ret;
	ret = create_execcmd();
	cmd = (struct execcmd*)ret;

	argc = 0;
	ret = get_redirection(ret, ps, es);

	while (!peek(ps, es, "|")) {
		if ((tok = get_token(ps, es, &q, &eq)) == 0)
			break;
		if (tok == '&')
			break;
		if(tok != 'a') {
			fprintf(stderr, "syntax error\n");
			exit(-1);
		}
		cmd->argv[argc] = copy_token(q, eq);
		argc++;
		if(argc >= MAXARGS) {
			fprintf(stderr, "too many args\n");
			exit(-1);
		}

		ret = get_redirection(ret, ps, es);
		if (peek(ps, es, ";")) {
			get_token(ps,es, 0, 0);
			cmd->next = get_pipe(ps, es);
		}
		if (peek(ps, es, ")")) // interesting case: redirection after the sequence of operation in parentheses
			break;
	}
	cmd->argv[argc] = 0;
	return ret;
}