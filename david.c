#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "parser.h"

 
int main() {
	//Init vars
	char input[1024];
	tline *tokens;
	int exit = 0;
	do{
		printf("msh>>");
		if(fgets(input, sizeof(input), stdin)){
			tokens = tokenize(input);
		}
		if((tokens->commands==NULL)||(tokens==NULL)){  //Tokenize fault. If not checked it will lead to segmentation fault in the case of empty redirect		
		} else {
			executeLine (tokens);
			wait(NULL);
		}
	} while (!exit); //Ask for inputs until exit = true
}

int executeLine(tline *line){
	//init Var
	int stdoutCopy = 1;
	int stdinCopy = 0;
	int stdErrorCopy = 2;
	int status;
	pid_t pid;

	//check input redirection
	if (line->redirect_input == NULL){
	}else{
		int file = open(line->redirect_input, O_RDONLY);
		if(file < 0) {
			printf("Input redirect error: File not found");
			return 1;
		}
		int stdinCopy = dup(0);                // Clone stdout to a new descriptor
		if(dup2(file, 0) < 0) return 1;         // Change stdout to file
		close(file);
	}

	//check output redirection
	if (line->redirect_output == NULL){
	}else{
		int file = open(line->redirect_output, O_WRONLY | O_CREAT | O_TRUNC);
		if(file < 0) return 1;
		int stdoutCopy = dup(1);                // Clone stdout to a new descriptor
		if(dup2(file, 1) < 0) return 1;         // Change stdout to file
		close(file);
		printf("%i", stdoutCopy);
	}

	//check error output redirection
	if (line->redirect_error == NULL){
	}else{
		int file = open(line->redirect_output, O_WRONLY | O_CREAT | O_TRUNC);
		if(file < 0) return 1;
		int stdErrorCopy = dup(2);                // Clone stdout to a new descriptor
		if(dup2(file, 2) < 0) return 1;         // Change stdout to file
		close(file);
	}

	//Check if there're pipes
	if (line->ncommands>1){
		forkPipes(line->ncommands, line->commands);
		waitpid(-1, NULL, 0);
	} else {
		if ((pid = fork())==0){
			return execvp (line->commands[0].argv[0], line->commands[0].argv);
		} else {
			waitpid(pid,&status,WUNTRACED);
		}
	}


	//Close and return
	//dup2(stdinCopy, 0);
	//dup2(stdoutCopy, 1);
	//dup2(stdErrorCopy, 2);
	
	return 0;
}

int forkPipes (int n, tcommand *commands){	
	int i, status;
	pid_t pid;
	int in, out, fd [2];
	// First command uses the original FD
	in = 0;

	//Spawn all but the very last one
	for (i = 0; i < n - 1; ++i){
		pipe (fd);
		out = fd[1]; //out is the write end of the pipe
		spawnProc (in, fd [1], commands + i);
		close (out);

		//Save the read end of the pipe, the next child will read from there
		in = fd [0];
	}

	//Execute the last one
	spawnProc (in, 1, (commands+n)-1);
	return 0;
}


int spawnProc (int in, int out, tcommand *command){
	pid_t pid;
	int status;
	if ((pid = fork ()) == 0){
		if (in != 0){
			dup2 (in, 0);
			close (in);
		}

		if (out != 1){
			dup2 (out, 1);
			close (out);
		}
		return execvp(command->argv[0],command->argv);
	} else if (pid < 0 ){
		fprintf(stderr,"Fork error");
	}
	return pid;
}
