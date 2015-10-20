#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
static void sigIgnore(int sig) {}
int main(int argc, char **argv)
{
	const char *home = getenv("HOME");
	const int homelen = strlen(home);
	signal(SIGCHLD, SIG_DFL);
	struct sigaction si = { .sa_handler = sigIgnore, .sa_flags = SA_RESTART };
	sigemptyset(&si.sa_mask);
	sigaction(SIGALRM, &si, 0);
	sigaction(SIGUSR1, &si, 0);
	sigset_t mask, old;
	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR1);
	sigprocmask(SIG_BLOCK, &mask, &old);
	pid_t serverpid = fork();
	if (!serverpid) {
		sigprocmask(SIG_SETMASK, &old, 0); // Unblock
		signal(SIGTTIN, SIG_IGN);
		signal(SIGTTOU, SIG_IGN);
		signal(SIGUSR1, SIG_IGN);
		setpgid(0,getpid());
		char *xserverrc[4] = {"sh", 0, ":0", 0};
		if (!(xserverrc[1] = getenv("XSERVERRC")) && home) {
			xserverrc[1] = malloc(homelen+strlen("/.xserverrc")+1);
			memcpy(xserverrc[1], home, homelen);
			memcpy(xserverrc[1]+homelen, "/.xserverrc", strlen("/.xserverrc")+1);
		}
		execvp(xserverrc[1], xserverrc+1);
		execvp("sh", xserverrc);
		_exit(EXIT_FAILURE);
	}
	if (~serverpid){
		waitpid(serverpid, 0, WNOHANG);
		alarm(8); // kludge to avoid tcp race
		sigsuspend(&old);
		alarm(0);
		sigprocmask(SIG_SETMASK, &old, 0);
	}
	pid_t clientpid = fork();
	if (!clientpid){
		if (setenv("DISPLAY", ":0", true) == -1) fputs("cannot set DISPLAY", stderr);
		else if (setuid(getuid()) == -1) fputs("cannot setuid", stderr);
		else{
			setpgid(0, getpid());
			char *xinitrc[3] = {"sh", 0, 0};
			if (!(xinitrc[1] = getenv("XINITRC")) && home) {
				xinitrc[1] = malloc(homelen+strlen("/.xinitrc")+1);
				memcpy(xinitrc[1], home, homelen);
				memcpy(xinitrc[1]+homelen, "/.xinitrc", strlen("/.xinitrc")+1);
			}
			execvp(xinitrc[1], xinitrc+1);
			execvp("sh", xinitrc);
		}
		_exit(EXIT_FAILURE);
	}
	if (serverpid > 0 && clientpid > 0) {
		pid_t pid;
		do pid = wait(0); while (pid != clientpid && pid != serverpid);
	}
	if (clientpid > 0) killpg(clientpid, SIGHUP);
	else if (clientpid < 0) fputs("client error", stderr);
	if (!(serverpid >= 0 && killpg(serverpid, SIGTERM) && killpg(serverpid, SIGKILL))) fputs("server error", stderr);
}
