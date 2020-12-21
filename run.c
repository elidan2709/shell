#include "run.h"

int getcmd(char *buf, int nbuf)
{
	if (isatty(fileno(stdin))) {
		fprintf(stdout, "> ");
		fflush(stdout);
	}
	memset(buf, 0, nbuf);
	/*reads in at most one less than size characters from stream.
	 * '\0' is inserted in the end of buf*/
	if (!fgets(buf, nbuf, stdin)) {
		fprintf(stderr, "Error in reading data\n");
		exit(-1);
	}
	if(buf[0] == 0) // EOF
		return -1;
	return 0;
}

void runcmd(struct cmd *cmd)
{
	if(cmd == 0)
		exit(0);

	switch(cmd->type){
	default:
		fprintf(stderr, "unknown type of command\n");
		exit(-1);
	case ' ':
		run_pure_command(cmd);
		break;
	case '^':
	case '>':
	case '<':
		run_redirected_command(cmd);
		break;
	case '|':
		run_pipe_command(cmd);
		break;
	}
	exit(0);
}

void run_pipe_command(struct cmd *cmd)
{
	struct pipecmd *pcmd;
	pcmd = (struct pipecmd*)cmd;
	int pipefd[2];
	if (pipe(pipefd) < 0) {
		fprintf(stderr, "Error in pipe\n");
		exit(-1);
	}

	if (fork1() == 0) {
		if (close(pipefd[1]) < 0) {
			fprintf(stderr, "close write end in a child\n");
			exit(-1);
		}
		exec_pipe_connection(pipefd[0], STDIN_FILENO, pcmd->right);
	}

	if (close(pipefd[0]) < 0) {
		fprintf(stderr, "close read end in parent\n");
		exit(-1);
	}
	exec_pipe_connection(pipefd[1], STDOUT_FILENO, pcmd->left);
}

void run_redirected_command(struct cmd *cmd)
{
	struct redircmd *rcmd;
	rcmd = (struct redircmd*)cmd;
	int file = open(rcmd->file, rcmd->mode, (rcmd->type == '>') ? PERMISSION : 0);
	if (file < 0) {
		fprintf(stderr, "error in opening file %s; errno == %d\n", rcmd->file, errno);
		exit(-1);
	}

	if (file != rcmd->fd) {
		if (close(rcmd->fd) < 0) {
			fprintf(stderr, "erorr in closing standart file with fd = %d\n", rcmd->fd);
			exit(-1);
		}
		if (dup2(file, rcmd->fd) != rcmd->fd) {
			fprintf(stderr, "error in dup2\n");
			exit(-1);
		}
		if (close(file) < 0) {
			fprintf(stderr, "Error in closing opened file des\n");
			exit(-1);
		}
	}
	runcmd(rcmd->cmd);
}

void run_pure_command(struct cmd *cmd)
{
	struct execcmd *ecmd;
	ecmd = (struct execcmd*)cmd;
	if(ecmd->argv[0] == 0)
		exit(0);
	int ret = fork1();
	if (ret == 0) {
		execvp(ecmd->argv[0], ecmd->argv);
		fprintf(stderr, "error in exec errno code:  %d\n", errno);
	}
	else {
		wait(NULL);
		//change stdout and stdin file pointers to point to correct destination
		if (!isatty(STDIN_FILENO)) {
			reset_standard_fd("r", stdin);
			assert(isatty(STDIN_FILENO));
		}
		if (!isatty(STDOUT_FILENO)) {
			reset_standard_fd("a", stdout);
			assert(isatty(STDOUT_FILENO));
		}
		runcmd(ecmd->next);
	}
}

void reset_standard_fd(const char *mode, FILE *stream)
{
	FILE *tty = freopen("/dev/tty", mode, stream);
	if (tty == NULL) {
		fprintf(stderr, "error in opening terminal errno == %d\n", errno);
		exit(-1);
	}
}

void exec_pipe_connection(int pipefd, int type, struct cmd *cmd)
{
	if (pipefd != type) {
		if (dup2(pipefd, type) != type) {
			fprintf(stderr, "dup2 to change %d\n", type);
			exit(-1);
		}
		if (close(pipefd) < 0) {
			fprintf(stderr, "close(pipefd)");
			exit(-1);
		}
	}
	runcmd(cmd);
}

int fork1(void)
{
	int pid = fork();
	if(pid == -1)
		perror("fork");
	return pid;
}