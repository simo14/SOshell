#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include "parser.h"

//Constants
static const char PROMPT[] = "msh>>";

//Global vars
int fgPID;

//Signal handlers

void sigintHandler (int sig) {
	if (getpid() == fgPID) {
		exit(EXIT_FAILURE);
	}
}
void sigquitHandler (int sig) {
	if (getpid() == fgPID) {
		exit(EXIT_FAILURE);
	}
}

int main() {
	//Init vars
	char input[1024];
	tline *tokens;
	int exit = 0;

	//signal(SIGINT, sigintHandler);
	signal(SIGQUIT, sigquitHandler);

	do{
		printf(PROMPT);
		if(fgets(input, sizeof(input), stdin)){
			tokens = tokenize(input);
		}
		if((tokens->commands==NULL)||(tokens==NULL)){  //Tokenize fault. If not checked it will lead to segmentation fault in the case of empty redirect		
		} else {
			executeLine (tokens);
		}
	} while (!exit); //Ask for inputs until exit = true
}

int executeLine(tline *line){
	//init Var
	int stdoutRedir = 1;
	int stdinRedir = 0;
	int stdErrorRedir = 2;
	int status;
	pid_t pid;

	//check input redirection
	if (line->redirect_input == NULL){
	}else{
		int file = open(line->redirect_input, O_RDONLY);
		if(file < 0) {
			fprintf(stderr,"Input redirect error: File not found");
			return 1;
		}
		stdinRedir = file;
	}

	//check output redirection
	if (line->redirect_output == NULL){
	}else{
		int file = open(line->redirect_output, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
		if(file < 0) {
			fprintf(stderr,"Output redirect error: File not found");
			return 1;
		}
		stdoutRedir = file;
	}

	//check error output redirection
	if (line->redirect_error == NULL){
	}else{
		int file = open(line->redirect_output, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
		if(file < 0) if(file < 0) {
			fprintf(stderr,"Error output redirect error: File not found");
			return 1;
		}
		stdErrorRedir = file;
	}

	//Check if there're pipes
	if (line->ncommands>1){
		fgPID = forkPipes(stdinRedir, stdoutRedir, stdErrorRedir, line->ncommands, line->commands);
	} else {
		fgPID = forkPipes(stdinRedir, stdoutRedir, stdErrorRedir, line->ncommands, line->commands);
		
	}
	waitpid(fgPID, &status, 0);
	fgPID = 0;

	//Close and return
	if (stdinRedir != 0) close (stdinRedir);
	if (stdoutRedir != 1) close (stdoutRedir);
	if (stdErrorRedir != 2) close (stdErrorRedir);
	
	return 0;
}

int forkPipes (int inFD, int outFD, int errFD, int n, tcommand *commands){	
	int i, status;
	int pid;
	int in, out, fd [2];
	// First command uses the original FD
	in = inFD;

	//Spawn all but the very last one
	for (i = 0; i < n - 1; ++i){
		pipe (fd);
		out = fd[1]; //out is the write end of the pipe
		spawnProc (in, fd [1], errFD, commands + i);
		close (out);

		//Save the read end of the pipe, the next child will read from there
		in = fd [0];
	}

	//Execute the last one and return it's PID
	return spawnProc (in, outFD, errFD, (commands+n)-1);
}


int spawnProc (int in, int out, int err, tcommand *command){
	int pid;
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
		
		if (err != 2){
			dup2 (err, 2);
			close (err);
		}
		
		return execvp(command->argv[0],command->argv);
	} else if (pid < 0 ){
		fprintf(stderr,"Fork error");
	}
	return pid;
}


