#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "processList.h"
	

	struct tprocessList * newList(){
		struct tprocessList * list;
		list = malloc(sizeof(struct tprocessList));
		list->size = 0;
		list->first = NULL;
		list->last = NULL;
		return list;
	}

	int removeProcess (struct tprocessList * list, int n){
		if (list==NULL) return -1;
		if ((list->size)==0) return -1;
		struct tprocess * process;
		struct tprocess * process2;
		process = list->first;
		if ((n)==0){
			list->first = list->first->next;
			free(process);
			list->size = (list->size)-1;
			return 0;
		}		
		if ((list->size)>n){
			int i;
			for (i = 0; i < n; i = i + 1){
				process = process->next;
			}
			process2 = process->next;
			process->next = process2->next;
			free(process2);
			if ((process->next)==NULL) list->last = process;
			return 0;
		}
	}

	int addProcess (struct tprocessList * list, int pid, char * commands){
		if (list == NULL) return -1;
		
		struct tprocess * process = malloc(sizeof(struct tprocess));
		process->pid = pid;
		strcpy(process->commands, commands);
		process->next = NULL;

		if ((list->size) > 0){
			list->last->next = process;
			list->last = process;
			list->size = (list->size)+1;
			return (list->size)-1;
		} else {
			list->first = process;
			list->last = process;
			list->size = 1;
			return 1;
		}
		
	}
