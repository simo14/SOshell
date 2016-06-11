int executeLine(tline *line, char * command);
int* forkPipes(int inFD, int outFD, int errFD, int n, tcommand *commands);
int getSingleTextStatus(char * text, int pid);
int getTextStatus(char * text, struct tsequence *sequence);
int jobs(struct tprocessList * list);
int toFG (int toFG);	//fg
void sigintHandler(int sig);
void sigquitHandler(int sig);
void sigtstpHandler(int sig);
int spawnProc(int in, int out, int err, tcommand *command);
