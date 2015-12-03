#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

int main () {
	int i,j;
	int pid;

	int pipeta[2][2];
	//Hacer los pipes
	for(i=0; i<2; i++){
		pipe(pipeta[i]);
	}
	int aux[2];
	int aux2[2];
	/*pipe(aux);
	pipe(aux2);*/
	int in = STDIN_FILENO;
	
	for(j=0;j<2;j++) {
		pipe(aux);
		pid=fork();
		if(pid==0){
			dup2(in,STDIN_FILENO);
			dup2(aux[1], STDOUT_FILENO);
			if(j==0){
				//execlp("ls","ls", NULL);
				execlp("echo","echo","asp", NULL);
			}else if(j==1){
				execlp("grep","grep","p",NULL);
				//execlp("echo","echo","asp", NULL);
			}
			/*else if(j==2){
				execlp("grep","grep","p",NULL);
			}*/
		}else{	//padre
			close(in);
			in=aux[0];
		}
	}
	dup2(in,STDIN_FILENO);
	execlp("grep","grep","p",NULL);
	wait(NULL);
	return(0);
//https://gist.github.com/zed/7540510
}
