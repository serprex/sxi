#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
int main(int argc, char **argv){
	sigset_t sset;
	sigemptyset(&sset);
	sigaddset(&sset, SIGUSR1);
	sigprocmask(SIG_BLOCK, &sset, 0);
	const char *const home = getenv("HOME");
	const int homelen = strlen(home);
	char xrcpath[homelen+strlen("/.xserverrc")+1];
	char *const xrc[2] = {xrcpath, 0};
	memcpy(xrcpath, home, homelen);
	memcpy(xrcpath+homelen, "/.xserverrc", strlen("/.xserverrc")+1);
	const pid_t serverpid = fork();
	if (!serverpid) {
		signal(SIGUSR1, SIG_IGN);
		_exit(setpgid(0, getpid()) || execvp(xrcpath, xrc));
	}
	memcpy(xrcpath+homelen+3, "initrc", strlen("initrc")+1);
	do sigpending(&sset); while (!sigismember(&sset, SIGUSR1));
	const pid_t clientpid = fork();
	if (!clientpid) _exit(setuid(getuid()) || setpgid(0, getpid()) || execvp(xrcpath, xrc));
	pid_t pid;
	do pid = wait(0); while (pid != clientpid && pid != serverpid);
	killpg(clientpid, SIGHUP);
	_exit(killpg(serverpid, SIGTERM) && killpg(serverpid, SIGKILL));
}
