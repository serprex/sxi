#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
static void sigIgnore(int sig) {}
int main(int argc, char **argv)
{
	struct sigaction si = { .sa_handler = sigIgnore, .sa_flags = SA_RESTART };
	sigemptyset(&si.sa_mask);
	sigaction(SIGUSR1, &si, 0);
	const char *const home = getenv("HOME");
	const int homelen = strlen(home);
	char *const xrc[3] = {"sh", malloc(homelen+strlen("/.xserverrc")+1), 0};
	memcpy(xrc[1], home, homelen);
	const pid_t serverpid = fork();
	if (!serverpid) {
		signal(SIGTTIN, SIG_IGN);
		signal(SIGTTOU, SIG_IGN);
		signal(SIGUSR1, SIG_IGN);
		setpgid(0,getpid());
		memcpy(xrc[1]+homelen, "/.xserverrc", strlen("/.xserverrc")+1);
		execvp(xrc[1], xrc+1);
		execvp("sh", xrc);
		_exit(EXIT_FAILURE);
	}
	if (serverpid>=0){
		waitpid(serverpid, 0, WNOHANG);
	}
	const pid_t clientpid = fork();
	if (!clientpid){
		if (setuid(getuid()) == -1) fputs("cannot setuid", stderr);
		else{
			setpgid(0, getpid());
			memcpy(xrc[1]+homelen, "/.xinitrc", strlen("/.xinitrc")+1);
			execvp(xrc[1], xrc+1);
			execvp("sh", xrc);
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
