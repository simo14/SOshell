#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "processList.h"
	

	struct tprocessList * newList() {
		struct tprocessList * list;
		list = malloc(sizeof(struct tprocessList));
		list->size = 0;
		list->first = NULL;
		list->last = NULL;
		return list;
	}

	int removeProcess (struct tprocessList * list, int n) {
		if (list==NULL) return -1;
		if ((list->size)==0) return -1;
		struct tsequence * process;
		struct tsequence * process2;
		process = list->first;
		if ((n)==0) {
			list->first = list->first->next;
			free(process->pids);
			free(process);
			list->size = (list->size)-1;
			return 0;
		}		
		if ((list->size)>n) {
			int i;
			for (i = 0; i < n-1; i = i + 1){
				process = process->next;
			}
			process2 = process->next;
			process->next = process2->next;
			if ((process->next)==NULL) list->last = process;
			free(process2->pids);
			free(process2);
			return 0;
		}
	}

	int addProcess (struct tprocessList * list, int *pids, char * commands) {
		if (list == NULL) return -1;
		
		struct tsequence * sequence = malloc(sizeof(struct tsequence));
		sequence->pids = pids;
		strcpy(sequence->commands, commands);
		sequence->next = NULL;

		if ((list->size) > 0) {
			list->last->next = sequence;
			list->last = sequence;
			list->size = (list->size)+1;
			return (list->size)-1;
		} else {
			list->first = sequence;
			list->last = sequence;
			list->size = 1;
			return 1;
		}
		
	}

	struct tsequence * getProcess ( struct tprocessList * list, int position) {
		struct tsequence * sequence;
		int i = 0;

		if ((list == NULL)||(list->size==0)||(position > list->size)) return NULL;

		sequence = list->first;
		while (sequence!=NULL) {
			if (position == i) return sequence;
			i = i + 1;
			sequence = sequence->next;
		}
		return NULL;
	}

	struct tsequence * deepCopy (struct tsequence * origin) {
		struct tsequence * destination = malloc(sizeof(struct tsequence));
		destination->ncommands = origin->ncommands;
		strcpy(destination->commands, origin->commands);
		destination->next = destination->next;
		int len = destination->ncommands;
		int * newpids = malloc(len * sizeof(int));
   		int i;
		for (i = 0; i<len;i++){
			newpids[i] = origin->pids[i];		
		}
		destination->pids = newpids;
	}
