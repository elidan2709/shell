#include "run.h"
#include "processing.h"

struct cmd *get_pipe(char **, char *);
struct cmd *get_parentheses(char **, char *);
struct cmd *get_exec(char **, char *);
struct cmd* get_redirection(struct cmd *cmd, char **ps, char *es);

struct cmd* create_pipecmd(struct cmd *left, struct cmd *right);
struct cmd* create_redir(struct cmd *subcmd, char *file, int type);
struct cmd* cur_command(struct cmd *current);
struct cmd * change_redirection_in_sequence(char **ps, const char *es, struct cmd *cmd, int tok);
