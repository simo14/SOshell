#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/limits.h>
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
int deadProcesses[1024]; //store the positions of dead Processes in processes list
int deadProcessesSize;

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
	memset(deadProcesses, 0, MAXSIZE*sizeof(int));
	deadProcessesSize = 0;

	signal(SIGINT, sigintHandler);
	signal(SIGQUIT, sigquitHandler);
	signal(SIGTSTP, sigtstpHandler);

	do{
		printf(PROMPT);
		if(fgets(input, sizeof(input), stdin)){
			tokens = tokenize(input);
		}
		if((tokens==NULL)||(tokens->commands==NULL)){//Tokenize fault. Can lead to segmentation fault if ignored
		} else if (tokens->commands->filename==NULL){//Tokenize can't find a path: It's wrong typed or it's internal command. 		
			char * ownCommand;
			const char s[2] = " ";
			input[strcspn(input, "\n")] = 0;		
			ownCommand = strtok(input, s);
			if (strcmp("cd",ownCommand)==0){
				printf("Doing 'cd' command \n");
				ownCommand = strtok(NULL, s); //cd sin implementar, pero ownCommand contiene ya la ruta que introduce el usuario
			} else if (strcmp("jobs",ownCommand)==0) {
				jobs(processList);
			} else if (strcmp("fg",ownCommand)==0) {
				printf("Doing 'fg' command \n");
			} else if (strcmp("quit",ownCommand)==0) {
				exit = 1;
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
	return 0;
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

/*SHOW JOBS*/
int jobs (struct tprocessList * list){
	if ((list==NULL)||(list->size==0)){
	} else {
		int i = 0;
		char status[1024];
		char command[1024];

		//Clean processes from previous fg
		for (i = 0;i<deadProcessesSize;i=i+1){
			removeProcess (list,deadProcesses[i]-i);
		}
		memset(deadProcesses, 0, MAXSIZE*sizeof(int));
		deadProcessesSize = 0;

		i=0;
		struct tprocess * process;
		process = list->first;
		while(process!=NULL){
			if(getTextStatus(status,process->pid)==1){
				deadProcesses[deadProcessesSize]=i;
				deadProcessesSize = deadProcessesSize+1;
			}
			strcpy(command,process->commands);
			process=process->next;
			i = i+1;
			printf("[%d] %s -------- %s\n",i,status,command);
		}
	}
}

/* RETURN TEXTUAL INFORMATION ABOUT THE STATUS OF A PROCESS GIVEN ITS PID*/
int getTextStatus(char * text, int pid){
	char filepath[PATH_MAX];
	FILE * fp;
	char * line = NULL;
	size_t len = 0;
	ssize_t read;
	char * token;
	snprintf(filepath,PATH_MAX,"/proc/%d/stat", pid);
	fp = fopen(filepath,"r");
	if (fp == NULL) exit(EXIT_FAILURE);

	if ((read = getline(&line, &len, fp)) != -1) {
		token = strtok(line, " ");
		token = strtok(NULL, " ");
		token = strtok(NULL, " ");
		if (strcmp("R",token)==0){
			strcpy(text,"Running");
			fclose(fp);
			return 0;
		} else if (strcmp("S",token)==0) {
			strcpy(text,"Sleeping");
			fclose(fp);
			return 0;
		} else if (strcmp("D",token)==0) {
			strcpy(text,"Waiting");
			fclose(fp);
			return 0;
		}else if (strcmp("T",token)==0) {
			strcpy(text,"Stopped");
			fclose(fp);
			return 0;
		}else if (strcmp("t",token)==0) {
			strcpy(text,"Tracing");
			fclose(fp);
			return 0;
		}else if (strcmp("W",token)==0) {
			strcpy(text,"Paging");
			fclose(fp);
			return 0;
		}else if (strcmp("X",token)==0) {
			strcpy(text,"Dead");
			fclose(fp);
			return 1;
		}else if (strcmp("x",token)==0) {
			strcpy(text,"Dead");
			fclose(fp);
			return 1;
		}else if (strcmp("K",token)==0) {
			strcpy(text,"Wakekill");
			fclose(fp);
			return 1;
		}else if (strcmp("W",token)==0) {
			strcpy(text,"Waking");
			fclose(fp);
			return 0;
		}else if (strcmp("W",token)==0) {
			strcpy(text,"Parked");
			fclose(fp);
			return 0;
		}else if (strcmp("Z",token)==0) {
			int status = 0;			
			waitpid(pid,&status,WNOHANG);
			if(WIFEXITED(status)==0){
				strcpy(text,"Failed");
			} else{
				strcpy(text,"Done");
			}
			fclose(fp);
			return 1;
		}
    	}
	fclose(fp);
}

