#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include "log.h"

int savelog(const char* filename) {
	// Open file for append and setup time and pointer to list log
	FILE* file_log = fopen(filename, "a");
	list_log* curr = headptr;
	struct tm* tp;

	// Make sure file is opened
	if (file_log == NULL) {
		errno = EIO;
		return -1;
	}

	// Iterate over the log list until we reach the ending nullptr
	while (curr != NULL) {
		// get time in proper format and print formatted string to file
		tp = localtime(& curr->item.time);
		
		// If non-type don't output type
		if (curr->item.type == non_type) {
			fprintf(file_log, "%.2d:%.2d:%.2d\t%s\n", tp->tm_hour, tp->tm_min, tp->tm_sec, curr->item.string);
		}
		else {
			fprintf(file_log, "%.2d:%.2d:%.2d\t%c\t%s\n", tp->tm_hour, tp->tm_min, tp->tm_sec, curr->item.type, curr->item.string); 
		}
		curr = curr->next;
	}
	fclose(file_log);
	
	return 0;
}

