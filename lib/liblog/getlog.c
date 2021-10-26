/* AUTHOR: Thomas Hopkins
   DATE: 9/9/2021
   FOR PROF. BHATIA CMPSCI 4760 UMSL

*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "log.h"

char* getlog() {
	long unsigned int memsize = 0;
	list_log* listlog_item = headptr;
	data_t curritem;
	
	// Calculate needed memory space by adding all string item sizes
	for (int i = 0; i < listlog_size; i++) {	
		curritem = listlog_item->item;
		// 12 more characters for: "hh:mm:ss T \n"
		memsize += (12 + strlen(curritem.string)) * sizeof(char);
		listlog_item = listlog_item->next;
	}

	// Try to allocate the memory
	char* log = (char*)calloc(1, memsize);
	if (log == NULL) {
		errno = ENOMEM;
		return NULL; // Could not allocate memory
	}
	
	struct tm* tp;
	
	// Reset item to head and append all log messages for return 
	listlog_item = headptr;
	for (int i = 0; i < listlog_size; i++) {
		curritem = listlog_item->item;
		// Allocate memory for temporary formatted string for all log data 
		char* tmp = (char*)calloc(1, (12 + strlen(curritem.string)) * sizeof(char));
		tp = localtime(& curritem.time);
		sprintf(tmp, "%.2d:%.2d:%.2d %c %s\n", tp->tm_hour, tp->tm_min, tp->tm_sec, curritem.type, curritem.string);
		// Append log message to end
		strncat(log, tmp, strlen(tmp) * sizeof(char));
		listlog_item = listlog_item->next;
		free(tmp);
	}

	return log;
}

