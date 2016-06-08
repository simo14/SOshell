#ifndef processList_H_INCLUDED		//avoids including the same header file twice in the same compilation
#define processList_H_INCLUDED
	struct tsequence {		//a sequence of one or more commands
		char commands[1024];
		int pids[1024];
		struct tsequence *next;
	};

	struct tprocessList {
		struct tprocess *first;
		struct tprocess *last;
		int size;
	};
	
	struct tprocessList * newList();
	int removeProcess (struct tprocessList * list, int n);
	int addProcess (struct tprocessList * list, tsequence, char * commands);
	struct tsequence * getProcess (struct tprocessList * list, int position);
	
#endif
