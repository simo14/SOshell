#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main () {
	int i;
	int n = 5;	//5 procesos
	int pid[6];

	int pipeta[n][2];
	//Hacer los pipes
	for(i=0; i<n-1; i++){
		pipe(pipeta[i]);
	}
	for(i = 0; i<n; i++){
		pid[i] = fork();
		if(pid[i]==0){
			if((i<n-1)&&(i>0)){
				dup2(pipeta[0][i-1],0);
				dup2(pipeta[0][i+1],1);
			}
				printf("soy el hijo %d",i);
				exit(0);
		}
	}
	if(pid[2]==0){
		puts("la hemos cagado, los hijos ejecutan esta mierda");
	}
}
