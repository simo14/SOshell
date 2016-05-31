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
#include <string.h>
#include "parser.h"
#include "processList.h"

/* CONSTANTS */
static const char PROMPT[] = "msh>>";
static const int MAXSIZE = 1024;

/* GLOBAL VARIABLES */
int fgPID; // PID of the forefround process
char fgCommand[1024]; // Command string of the foreground process
int parentPID; // PID of the main process
struct tprocessList * processList; // List of background processes, in order to keep track of them.


/*SIGNAL HANDLERS*/
void sigintHandler (int sig) {
	if ((getpid() == parentPID) && (fgPID>0))  kill(fgPID,SIGINT); //kill the fg process
}
void sigquitHandler (int sig) {
	if ((getpid() == parentPID) && (fgPID>0)) kill(fgPID,SIGQUIT); //kill the fg process
}

void sigtstpHandler(int sig){
	if ((getpid() == parentPID) && (fgPID>0)) { //stop the fg process and add it to bg list
		kill(fgPID ,SIGTSTP);
		addProcess(processList, fgPID, fgCommand);
		fgPID = 0;
	}
}


/* MAIN LOOP: prints the prompt and reads the input */

int main() {
	//Init vars
	char input[MAXSIZE];
	int status;
	tline *tokens;
	parentPID = getpid();
	int exit = 0;
	processList = newList();

	//signal(SIGINT, sigintHandler); decomment once done "QUIT" command
	signal(SIGQUIT, sigquitHandler);
	signal(SIGTSTP, sigtstpHandler);

	do{
		printf(PROMPT);
		if(fgets(input, sizeof(input), stdin)){
			tokens = tokenize(input);
		}
		if((tokens==NULL)){//Tokenize fault. Can lead to segmentation fault if ignored
		} else if (tokens->commands->filename==NULL){//Tokenize can't find a path: It's wrong typed or it's internal command. 		
			char * ownCommand;
			const char s[2] = " ";
			input[strcspn(input, "\n")] = 0;		
			ownCommand = strtok(input, s);
			if (strcmp("cd",ownCommand)==0){
				printf("Doing 'cd' command \n");
				ownCommand = strtok(NULL, s);
			} else if (strcmp("jobs",ownCommand)==0) {
				printf("Doing 'jobs' command \n");
			} else if (strcmp("fg",ownCommand)==0) {
				printf("Doing 'jobs' command \n");
			}
		} else {
			input[strcspn(input, "\n")] = 0;
			executeLine (tokens, input);
		}
		
		//If a fg process has spawned -> wait for it to finish or to be stoped. Set fgPID to 0 'cause there's no active fg process anymore.
		if (fgPID>0) {		
			waitpid(fgPID, &status, WUNTRACED);
			fgPID = 0;
		}

	} while (!exit); //Ask for inputs until exit = true
}


/* READ AND EXECUTE A TOKENIZED LINE */
int executeLine(tline *line, char * command){
	//init Var
	int stdoutRedir = 1;
	int stdinRedir = 0;
	int stdErrorRedir = 2;
	int status;
	int pid;
	char * uhu;

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

	//For 0 to n pipes (works for standalone commands too!)	
	if ((line->background)==0){
		strcpy(fgCommand,command);
		fgPID = forkPipes(stdinRedir, stdoutRedir, stdErrorRedir, line->ncommands, line->commands);
	} else {
		addProcess(processList, forkPipes(stdinRedir, stdoutRedir, stdErrorRedir, line->ncommands, line->commands), command);
	}

	//Close and return
	if (stdinRedir != 0) close (stdinRedir);
	if (stdoutRedir != 1) close (stdoutRedir);
	if (stdErrorRedir != 2) close (stdErrorRedir);
	
	return 0;
}

/*FORK 1 to n commands using pipes: if there's just 1 command, forkPipes spawns it normally*/
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


/*SPAWN A SINGLE PROCESS GIVEN ITS INPUT, OUTPUT AND ERROR FILE DESCRIPTORS*/
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


