#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

int main () {
	int i;
	int n = 3;	//Number of processes
	int pid[n];

	int pipeta[n-1][2];
	//Hacer los pipes
	for(i=0; i<n-1; i++){
		pipe(pipeta[i]);
	}
	for(i = 0; i<n; i++){
		pid[i] = fork();
		if(pid[i]==0){
			if((i<n-1)&&(i>0)){				//Problema: hace un pipe cada vez. en i=0 hace el primer pipe y exec. la salida del pipe ni siquiera está asignada
				dup2(pipeta[i-1][0],0);
				printf("id %d comes from pipeta %d",i,i-1);
				dup2(pipeta[i][1],1);
			}else if(i==0){
				dup2(pipeta[i][1],1);
			}else if(i==n-1){
				dup2(pipeta[i-1][0],0);
			}
			/*if(i==0){
				execlp("ls","ls", NULL);
			}
			else{
				execlp("grep","grep","p", NULL);
			}
			printf("soy el hijo %d",i);
			exit(0);*/
		}
	}
	if(pid[0]==0){	//hijo original
		execlp("ls","ls", NULL);
	}
	else if (pid[n-1]==0){
		execlp("grep","grep","p", NULL);
	}
	if(pid[2]==0){
		puts("la hemos cagado, los hijos ejecutan esta mierda");
	}
}
