#ifndef processList_H_INCLUDED		//avoids including the same header file twice in the same compilation
#define processList_H_INCLUDED
	struct tprocess {
		char commands[1024];
		int pid;
		struct tprocess *next;
	};

	struct tprocessList {
		struct tprocess *first;
		struct tprocess *last;
		int size;
	};
	
	struct tprocessList * newList();
	int removeProcess (struct tprocessList * list, int n);
	int addProcess (struct tprocessList * list, int pid, char * commands);
	
#endif
