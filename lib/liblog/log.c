#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "log.h"

const char msg_types[] = {'I', 'W', 'E', 'N'};
const char fatal_types[] = {'F'};
const char non_type = 'N';
const int fataltypes_size = 1;
const int msgtypes_size = 4;

list_log* headptr = NULL;
list_log* tailptr = NULL;
int listlog_size = 0;

