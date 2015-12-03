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
	pipe(aux);
	pipe(aux2);
	
	for(j=0;j<3;j++) {
		pid=fork();
		if(pid==0){
			if(j==0){
				dup2(aux2[1],1);
				close(aux2[0]);
			}else if(j==1){
				//dup2(aux2[0],0);
				dup2(aux[1],1);	
			}else if(j==2){
				dup2(aux[0],0);
				close(aux2[1]);
			}
			if(j==0){
				execlp("ls","ls", NULL);
				//execlp("echo","echo","asp", NULL);
				//exit(0);
			}else if(j==1){
				dup2(aux2[0],0);
				execlp("grep","grep","p",NULL);
				//execlp("ls","ls", NULL);
				//execlp("echo","echo","asp", NULL);
			}
			else if(j==2){
				execlp("grep","grep","r",NULL);
			}
		}
	}
	wait(NULL);
	return(0);
//https://gist.github.com/zed/7540510
}
