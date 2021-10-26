#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include "log.h"

int isfataltype(const char type) {
	for (int i = 0; i < fataltypes_size; i++) {
		if (type == fatal_types[i]) return 1;
	}
	return 0;
}

int ismsgtype(const char type) {
	for (int i = 0; i < msgtypes_size; i++) {
		if (type == msg_types[i]) return 0;
	}
	if (isfataltype(type) > 0) return 0;
	return -1;
}

int addmsg(const char type, const char* msg) {
	// Validate message type
	if (ismsgtype(type) < 0) {
		errno = EINVAL;
		return -1;
	}
	// Declare the new item and it's size
	list_log* new_item;
	int item_size;

	// Initialize the item size
	item_size = sizeof(list_log) + strlen(msg) + 1;

	// Attempt to allocate space for the new item
	if ((new_item = (list_log *)(malloc(item_size))) ==  NULL) {
		// Failed to add new item
		errno = ENOMEM;
		return -1;
	}
	
	// Set item's data members
	new_item->item.time = time(NULL);
	new_item->item.type = type;
	new_item->item.string = (char *)malloc(strlen(msg) + 1);
	// Failed to allocate space for message
	if (new_item->item.string == NULL) {
		errno = ENOMEM;
		return -1;
	}
	// Copy the string into the item
	strcpy(new_item->item.string, msg);
	
	// set list ptrs 
	if (headptr == NULL) {
		// First element, set the headptr and point it to tailptr
		headptr = new_item;
		headptr->next = tailptr;
	} else {
		// append this item to back of list
		tailptr->next = new_item;
	}
	// Advance tailptr to end of the list
	tailptr = new_item;
	listlog_size++;

	// Check if fatal message
	if (isfataltype(type) > 0) {
		// Because we don't have a reference to the user defined filename
		// Lets let the calling program handle this.
		return 1;
	}
	
	return 0;
}

