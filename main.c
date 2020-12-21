#include <signal.h>
#include "run.h"
/*
 * Background mode is implemented in the way that a parent process who creates
 * a child to execute a command don't wait for its termination, but requests new input.
 * We need to care about removing a record of child process from process table in the operating system
 * for accomplishing we set a SIGCHLD disposition to wait for all child processes who changed the state.
 * */

void wait_dead_children(int sig);

int main(void)
{
	static char buf[100];
	int is_background;

	/*set SIGCHLD disposition*/
	struct sigaction act;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_RESTART;
	act.sa_handler = wait_dead_children;
	int res = sigaction(SIGCHLD, &act, NULL);
	if (res != 0) {
		fprintf(stderr, "error in setting disposition for SIGCHLD\n");
		exit(-1);
	}
	while(getcmd(buf, sizeof(buf)) >= 0) {
		if (strncmp(buf, EXIT_COMMAND, strlen(EXIT_COMMAND)) == 0)
			exit(0);
		is_background = detect_background(buf);
		if(fork1() == 0)
			runcmd(parsecmd(buf));
		if (!is_background)
			wait(NULL);
	}

	return 0;
}

void wait_dead_children(int sig)
{
	while (waitpid(-1, NULL, WNOHANG) > 0)
		continue;
}