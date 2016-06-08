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
#include "myshell.h"


/* CONSTANTS */
static const char PROMPT[] = "msh>>";
static const int MAXSIZE = 1024;

/* GLOBAL VARIABLES */
struct tsequence * fgSequence; 			// PIDs of the foregrounds processes
char fgCommand[1024]; 			// Command string of the foreground process
int parentPID; 				// PID of the main process
struct tprocessList * processList; 	// List of background processes, in order to keep track of them.
int deadProcesses[1024]; 		//store the positions of dead Processes in processes list
int deadProcessesSize;

/*SIGNAL HANDLERS*/
void sigintHandler (int sig) {
	if ((getpid() == parentPID) && (fgSequence->pids!=NULL)) {
		int i;
		for ( i = 0; i < fgSequence->ncommands; i++){	
			kill(fgSequence->pids[i],SIGINT);
		}
	}//kill the fg process
}
void sigquitHandler (int sig) {
	if ((getpid() == parentPID) && (fgSequence->pids!=NULL)) {
		int i;
		for ( i = 0; i < fgSequence->ncommands; i++){	
			kill(fgSequence->pids[i],SIGQUIT);
		}
	} //kill the fg process
}

void sigtstpHandler(int sig){
	if ((getpid() == parentPID) && (fgSequence->pids!=NULL)) { //stop the fg process and add it to bg list
		int i;
		for ( i = 0; i < fgSequence->ncommands; i++){	
			kill(fgSequence->pids[i],SIGTSTP);
		}
		addProcess(processList, fgSequence->pids, fgCommand);
		free (fgSequence);
		fgSequence = NULL;
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
	fgSequence = NULL;
	processList = newList();
	memset(deadProcesses, 0, MAXSIZE*sizeof(int));
	deadProcessesSize = 0;

	signal(SIGINT, sigintHandler);
	signal(SIGQUIT, sigquitHandler);
	signal(SIGTSTP, sigtstpHandler);

	do{
		printf(PROMPT);
		if(fgets(input, sizeof(input), stdin)!=NULL){
			tokens = tokenize(input);
		} else { //Holds EOF input
			tokens = NULL;
		}
		if((tokens==NULL)||(tokens->commands==NULL)){ //Tokenize fault. Can lead to segmentation fault if ignored
			fprintf(stdin, "\n");
		} else {
			int commandIsGood = 1;
			int contador = 0;
			char *failedCommand;
			char inputCopy[MAXSIZE];
			strcpy(inputCopy,input);
			while(contador<(tokens->ncommands)) {
				tcommand *commands = (tokens->commands)+contador;
				int newCheckIsGood = !((commands->filename) == NULL);
				if(!newCheckIsGood) {
					int cont=0;
					inputCopy[strcspn(inputCopy, "\n")] = 0;
					failedCommand = strtok(inputCopy,"|");
					while(failedCommand!=NULL && cont<contador){	
        					failedCommand=strtok(NULL,"");
       						cont++;
    					}
				}
				commandIsGood = commandIsGood && newCheckIsGood;
				contador++;
			}
			if(((tokens->commands->filename) == NULL)) {		//First command was not recognized by tokens
				char * ownCommand;
				char * arguments;
				const char s[2] = " ";
				input[strcspn(input, "\n")] = 0;		
				ownCommand = strtok(input, s);
				arguments = strtok(NULL, s);	//get the argument
				if (strcmp("cd",ownCommand)==0){
					if (arguments == NULL){
						if(chdir(getenv("HOME"))!=0){
							printf("Error: Ruta no válida");
						}
					} else {
						if(chdir(arguments)!=0){
							printf("Error: Ruta no válida");
						}
					}
				} else if (strcmp("jobs",ownCommand)==0) {
					jobs(processList);
				} else if (strcmp("fg",ownCommand)==0) {
					int toFG = atoi (arguments);
					struct tsequence * process = getProcess(processList, toFG);
					if (process!=NULL){				
						fgSequence=deepCopy(process);
						//fgSequence->ncommands = 1;
						strcpy(fgCommand, process->commands);
						removeProcess(processList, toFG);
						char statusFG[1024];
						int i;
						for ( i = 0; i < fgSequence->ncommands; i++){	
							kill(fgSequence->pids[i], SIGCONT);
						}
						//getTextStatus checks status for all processes of the sequence and returns 1 if all of them are finished already
						if ((getTextStatus(statusFG, fgSequence))==1){	
							free(fgSequence);							
							fgSequence = NULL;
							fprintf(stderr,"El processo [%i]+%s ya ha finalizado.\n", toFG, fgCommand);
						}
					} else {
						fprintf(stderr,"Error: No existe ningún proceso número %d\n", toFG);
					}
			
			
				} else if (strcmp("quit",ownCommand)==0) {
					exit = 1;
				} else {	//Command doesn't exist
					fprintf(stderr, "%s: No se encuentra el mandato.\n",ownCommand);
				}

			} else {	//The first command was not a special or invalid command
			
				if (commandIsGood) {
					input[strcspn(input, "\n")] = 0;
					fgSequence = malloc(sizeof(struct tsequence));
					strcpy(fgSequence->commands, input);		//we want the string without tokenize for later use of jobs
					fgSequence->ncommands = tokens->ncommands;
					fgSequence->next = NULL;
					executeLine (tokens, input);
				} else {	//One of the commands (no the first one) is a special or invalid one
					fprintf(stderr, "%s: No se encuentra el mandato.\n", failedCommand);
				}
			}
			//If a fg process has spawned -> wait for it to finish or to be stoped. Set fgSequence to 0 'cause there's no active fg process anymore.
			if (fgSequence!=NULL && fgSequence->pids!=NULL) {		//If pids is null it means than command was to be executed in background
				int i;		
				for ( i = 0; i < fgSequence->ncommands; i++){	
					waitpid(fgSequence->pids[i], &status, WUNTRACED);
				}
				free(fgSequence);
				fgSequence = NULL;
			}
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
	char * uhu;

	//check input redirection
	if (line->redirect_input == NULL){
	}else{
		int file = open(line->redirect_input, O_RDONLY);
		if(file < 0) {
			fprintf(stderr,"%s: Error. Archivo no encontrado.\n", line->redirect_input);
			return 1;
		}
		stdinRedir = file;
	}

	//check output redirection
	if (line->redirect_output == NULL){
	}else{
		int file = open(line->redirect_output, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
		if(file < 0) {
			fprintf(stderr,"%s: Error. No se ha podido crear el archivo.\n", line->redirect_output);
			return 1;
		}
		stdoutRedir = file;
	}

	//check error output redirection
	if (line->redirect_error == NULL){
	}else{
		int file = open(line->redirect_output, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
		if(file < 0) if(file < 0) {
			fprintf(stderr,"%s: Error. No se ha podido crear el archivo.\n",  line->redirect_error);
			return 1;
		}
		stdErrorRedir = file;
	}

	//For 0 to n pipes (works for standalone commands too!)	
	if ((line->background)==0){
		strcpy(fgCommand,command);
		fgSequence->pids = forkPipes(stdinRedir, stdoutRedir, stdErrorRedir, line->ncommands, line->commands);
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
int* forkPipes (int inFD, int outFD, int errFD, int n, tcommand *commands){	
	int i, status;
	int *pids = malloc(n * sizeof(int));		//For it to persist after function finishes, and so to be able to return it
	int in, out, fd [2];
	// First command uses the original FD
	in = inFD;

	//Spawn all but the very last one
	for (i = 0; i < n - 1; ++i){
		pipe (fd);
		out = fd[1]; //out is the write end of the pipe
		pids[i] = spawnProc (in, fd [1], errFD, commands + i);
		close (out);

		//Save the read end of the pipe, the next child will read from there
		in = fd [0];
	}

	//Execute the last one and return it's PID
	pids[n-1] = spawnProc (in, outFD, errFD, (commands+n)-1);
	return pids;
}


/*SPAWN A SINGLE PROCESS GIVEN ITS INPUT, OUTPUT AND ERROR FILE DESCRIPTORS*/
int spawnProc (int in, int out, int err, tcommand *command){
	int pid;
	int status;
	if ((pid = fork ()) == 0){
		setpgid(pid, 0); 
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
		struct tsequence * sequence;
		sequence = list->first;
		while(sequence!=NULL){
			if(getTextStatus(status, sequence)==1){
				deadProcesses[deadProcessesSize]=i;
				deadProcessesSize = deadProcessesSize+1;
			}
			strcpy(command,sequence->commands);
			sequence=sequence->next;
			i = i+1;
			printf("[%d]+  %s                    %s\n",i-1,status,command);
		}
	}
}

/* RETURNS TEXTUAL INFORMATION ABOUT THE STATUS OF A PROCESS GIVEN ITS PID. Returns 0 if alive, 1 if dead*/
int getSingleTextStatus(char * text, int pid){
	int returnValue;
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
			returnValue = 0;
		} else if (strcmp("S",token)==0) {
			strcpy(text,"Sleeping");
			returnValue = 0;
		} else if (strcmp("D",token)==0) {
			strcpy(text,"Waiting");
			returnValue = 0;
		}else if (strcmp("T",token)==0) {
			strcpy(text,"Stopped");
			returnValue = 0;
		}else if (strcmp("t",token)==0) {
			strcpy(text,"Tracing");
			returnValue = 0;
		}else if (strcmp("W",token)==0) {
			strcpy(text,"Paging");
			returnValue = 0;
		}else if (strcmp("X",token)==0) {
			strcpy(text,"Dead");
			returnValue = 1;
		}else if (strcmp("x",token)==0) {
			strcpy(text,"Dead");
			returnValue = 1;
		}else if (strcmp("K",token)==0) {
			strcpy(text,"Wakekill");
			returnValue = 1;
		}else if (strcmp("W",token)==0) {
			returnValue = 0;
		}else if (strcmp("W",token)==0) {
			strcpy(text,"Parked");
			returnValue = 0;
		}else if (strcmp("Z",token)==0) {
			strcpy(text,"Zombie");
			returnValue = 1;
		} else {
			strcpy(text,"Unknown");
			returnValue = 1;
		}
    	}
	fclose(fp);
	if (returnValue == 1) {
		int status = 0;			
		waitpid(pid,&status,WNOHANG);
		if(WIFEXITED(status)==0){
			strcpy(text,"Done");
		} else{
			strcpy(text,"Failed");
		}
	}
	return returnValue;
}
int getTextStatus(char * text, struct tsequence *sequence){
	int i,returnValue;
	returnValue = -1;
	if (sequence==NULL) return -1;		
	for ( i = 0; i < sequence->ncommands; i++){	
		returnValue = getSingleTextStatus(text, sequence->pids[i]);
		if (returnValue == 0){
			return returnValue;
		}
	}
	return returnValue;
}

