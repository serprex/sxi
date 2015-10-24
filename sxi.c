#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
int main(int argc, char **argv){
	signal(SIGUSR1, SIG_IGN);
	const char *const home = getenv("HOME");
	const int homelen = strlen(home);
	char xrcpath[homelen+strlen("/.xserverrc")+1];
	char *const xrc[2] = {xrcpath, 0};
	memcpy(xrcpath, home, homelen);
	memcpy(xrcpath+homelen, "/.xserverrc", strlen("/.xserverrc")+1);
	const pid_t serverpid = fork();
	if (!serverpid) {
		signal(SIGTTIN, SIG_IGN);
		signal(SIGTTOU, SIG_IGN);
		return setpgid(0, getpid()) || execvp(xrcpath, xrc);
	}else if (serverpid < 0) return fputs("server fork failed", stderr);
	const pid_t clientpid = fork();
	if (!clientpid){
		memcpy(xrcpath+homelen+3, "initrc", strlen("initrc")+1);
		return setuid(getuid()) || setpgid(0, getpid()) || execvp(xrcpath, xrc);
	} else if (clientpid < 0) return fputs("client fork failed", stderr);
	pid_t pid;
	do pid = wait(0); while (pid != clientpid && pid != serverpid);
	killpg(clientpid, SIGHUP);
	return killpg(serverpid, SIGTERM) && killpg(serverpid, SIGKILL);
}
