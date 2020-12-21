#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/wait.h>


#define MAXARGS 10
#define PERMISSION 0644
#define EXIT_COMMAND "exit"


// All commands have at least a type. Have looked at the type, the code
// typically casts the *cmd to some specific cmd type.
struct cmd {
	int type;          //  ' ' (exec), | (pipe), '<', '>', ">>" for redirection
};

struct execcmd {
	int type;              // ' '
	char *argv[MAXARGS];   // arguments to the command to be exec-ed
	struct cmd *next;
};

struct redircmd {
	int type;          // <, >, >> (substitution with '^')
	struct cmd *cmd;   // the command to be run (e.g., an execcmd)
	char *file;        // the input/output file
	int mode;          // the mode to open the file with
	int fd;            // the file descriptor number to use for the file
};

struct pipecmd {
	int type;          // |
	struct cmd *left;  // left side of pipe
	struct cmd *right; // right side of pipe
};

int getcmd(char *buf, int nbuf);
struct cmd* parsecmd(char *s);
void runcmd(struct cmd *cmd);
int detect_background(const char *buf);

int fork1(void);  // Fork but exits on failure.
void reset_standard_fd(const char *mode, FILE *stream);

void exec_pipe_connection(int pipefd, int type, struct cmd *cmd);

void run_pure_command(struct cmd *cmd);
void run_redirected_command(struct cmd *cmd);
void run_pipe_command(struct cmd *cmd);

int detect_background(const char * buf);
