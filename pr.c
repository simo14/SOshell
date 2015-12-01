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
	int n = 3;	//Number of processes
	int pid[n];

	int pipeta[n-1][2];
	//Hacer los pipes
	for(i=0; i<n-1; i++){
		pipe(pipeta[i]);
	}
	
	for(j=0;j<n;j++) {
		pid[j]=fork();
		if(pid[j]==0){
			if((j<n-1)&&(j>0)){
				dup2(pipeta[j-1][0],0);
				fprintf(stderr,"id %d comes from pipeta %d",j,j-1);
				dup2(pipeta[j][1],1);
			}else if(j==0){
				dup2(pipeta[j][1],1);
			}else if(j==n-1){
				fprintf(stderr,"holi%d-%d",j,n-1);
				dup2(pipeta[j-1][0],0);
			}
			if(j==0){
				fputs("lo hago ",stderr);
				execlp("ls","ls", NULL);
			}else{
				fputs("grep ",stderr);
				execlp("grep","grep","p", NULL);
			}
		}
	}
	wait(NULL);
	return(0);
//https://gist.github.com/zed/7540510
}
