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
			if(tokens==NULL){		//Tokenize fault. If not checked it will lead to 
							//segmentation fault in the case of empty redirect argument ex: "ls >"
				char stringnew[1024];
				printf("%.*s: No se encuentra el mandato\n", strlen(string)-1, string);
				continue;
			}
			if(tokens->ncommands>=1){
				int i=tokens->ncommands;
				while(i>0){
					pid = fork();
					if (pid<0) {
						fprintf(stderr,"Falló el fork");	
						continue;
					} else if (pid == 0) {			//Son process
						if(redireccion(tokens)!=0){
							continue;
						}
						execvp(tokens->commands->filename, tokens->commands->argv);	
						printf("Error al ejecutar el comando: %s\n", strerror(errno));
						exit(-1);
					} else {				//Father process
						wait(NULL);
						if(WIFEXITED(status)!=0) {	//Abnormal exit	
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
					i--;
				}
			}
		}else {	//TODO INPUT MALO PETA
				fprintf(stderr, "Input error.");
				continue;
			}
	}
	return 0;
}


int redireccion(tline *tokens) {
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	int fichero;
	if(tokens->redirect_output!=NULL){
		fichero = open(tokens->redirect_output, O_CREAT | O_WRONLY, mode);
		if(fichero<1){
			printf("%s: Error. No se ha podido crear o abrir el fichero especificado\n", tokens->redirect_output);
			return -1;
		}
		int out=dup(1);			//Security copy of stdout
		dup2(fichero,1);
	} else if(tokens->redirect_input!=NULL){
		fichero = open(tokens->redirect_input, O_RDONLY, mode);
		if(fichero<1){
			printf("%s: Error. No se ha podido abrir el fichero especificado para lectura\n", tokens->redirect_input);
			return -1;
		}
		int in=dup(0);			//Security copy of stdin
		dup2(fichero, 0);
	}
	close(fichero);
	return 0;
}
