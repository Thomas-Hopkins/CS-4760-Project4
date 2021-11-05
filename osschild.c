#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "shared.h"

static char* exe_name;

void help() {
    printf("Operating System Simulator Child usage\n");
    printf("Runs as a child of the OSS. Not to be run alone.\n");
}

int main(int argc, char** argv) {
    int option;
    int mode;
    exe_name = argv[0];

    while ((option = getopt(argc, argv, "hm:")) != -1) {
        switch (option)
        {
        case 'h':
            help();
            exit(EXIT_SUCCESS);
        case 'm':
            mode = optarg;
            break;
        case '?':
            // Getopt handles error messages
            exit(EXIT_FAILURE);
        }
    }

    // signal to add to process queue

    // wait until signaled to run

    // generate random time used and update sys clock with it
    // based on the mode of execution (io or cpu bound)

    // 
}