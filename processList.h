#ifndef processList_H_INCLUDED	//avoids including the same header file twice in the same compilation
#define processList_H_INCLUDED
	struct tsequence {	//a sequence of one or more commands, and also subceptible of being a node in a list of such sequences
		char commands[1024];
		int *pids;
		struct tsequence *next;
		int ncommands;
	};

	struct tprocessList {
		struct tsequence *first;
		struct tsequence *last;
		int size;
	};
	
	struct tprocessList * newList();
	int removeProcess (struct tprocessList * list, int n);
	int addProcess (struct tprocessList * list, int *pids, char * commands, int ncommads);
	struct tsequence * getProcess (struct tprocessList * list, int position);
	struct tsequence * deepCopy (struct tsequence * origin);
	
#endif
