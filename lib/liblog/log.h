#ifndef LOG_H
#define LOG_H
#include <time.h>

typedef struct data_struct {
	time_t time; // Time stamp
	char type; // Message type (I/W/E/F)
	char* string; // Message string
} data_t;

typedef struct list_struct {
	data_t item;
	struct list_struct* next;
} list_log;

extern const char msg_types[];
extern const char fatal_types[];
extern const char non_type;
extern const int fataltypes_size;
extern const int msgtypes_size;
extern list_log* headptr;
extern list_log* tailptr;
extern int listlog_size;

int addmsg(const char type, const char* msg);
void clearlog();
char* getlog();
int savelog(const char* filename);

#endif
