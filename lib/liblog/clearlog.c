#include <stdlib.h>
#include "log.h"

void clearlog() {
	// Start at list head and iteratively free each item's heap memory until end of list
	while (listlog_size > 0) {
		list_log* tmp = headptr;
		headptr = headptr->next;
		free(tmp->item.string);
		tmp = NULL;
		listlog_size--;
	}
	// Reset head and tail ptrs
	headptr = NULL;
	tailptr = NULL;
}

