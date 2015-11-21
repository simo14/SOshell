#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include "parser.h"


int main(int argc, char *argv[]) {
	pid_t pid;
	int status;
	char string[1024];
	tline *tokens;
	while(1){
		printf("msh> ");
		if(fgets(string, sizeof(string), stdin)){
			tokens = tokenize(string);

			
			mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
			int fichero;
			pid = fork();
			if (pid<0) {
				fprintf(stderr,"Falló el fork");	
				exit(-1);
			} else if (pid == 0) {		//Proceso hijo
				if(tokens->redirect_output!=NULL){
					fichero = open(tokens->redirect_output, O_CREAT | O_WRONLY, mode);
					int out=dup(1);			//copia de seguridad salida estandar
					dup2(fichero,1);
				} else if(tokens->redirect_input!=NULL){
					fichero = open(tokens->redirect_input, O_CREAT | O_RDONLY, mode);
					int in=dup(0);
					dup2(fichero, 0);
				}
				close(fichero);
				execvp(tokens->commands->filename, tokens->commands->argv);	
				fflush(stdout);
				printf("Error al ejecutar el comando: %s\n", strerror(errno));
				exit(-1);
			} else {				//Proceso padre
				wait(NULL);
				if(WIFEXITED(status)!=0) {	//salida anormal	
					if(WEXITSTATUS(status)!=0) {
						printf("El comando no se ha ejecutado correctamente");
					}else {
						exit(0);
					}
				printf("Se ha producido un error\n");
				printf("El comando no se ha ejecutado correctamente\n"); 
				exit(-1);
				}
			}
		}else {	//TODO INPUT MALO PETA
				fprintf(stderr, "Input error.");
				return -1;
			}
	}
}
