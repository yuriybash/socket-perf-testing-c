/* File of shared functions between the server and the client
 * Author: Yuriy Bash */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "client_server_utils.h"

/*
 *  Function: find_message_end
 *  --------------------------
 *  finds the index at which the newline character exists in the message
 *
 *  msg: pointer to start of message
 *  start: index at which to start search
 *
 *  returns: the index of the newline delimiter. -1 if newline not found.
 */
int find_message_end (char *msg, int start){

    char c;
    while((c = *(msg+start)) != '\0'){
        if(c == '\n'){
            return start;
        }
        start++;
    }

    return -1;
}

/*
 * Function: generate_message
 * --------------------------
 * given a buffer that contains 1-n messages delimited by '\n', return the
 * first message.
 *
 * messages: pointer to buffer
 * end: index at which first message ends (including the '\n' delimiter)
 *
 * returns: the first message (including its last char, '\n')
 *
 */
char *generate_message (char *messages, int end) {

    char* extracted_message = (char*) malloc((end+1)*sizeof(char));
    strncpy(extracted_message, messages, end);
    *(extracted_message + end) = '\0'; // not present on orig message in buffer

    int i = end;
    while(*(messages + i) != '\0'){
        *(messages + (i - end)) = *(messages + i);
        i++;
    }
    *(messages + (i - end)) = '\0';

    return extracted_message;

}

/*
 * Function: allocation_failed
 * ---------------------------
 *
 * clean up when a malloc fails.
 *
 * returns: void
 */
void allocation_failed () {
    fprintf (stderr, "Unable to allocate enough memory\n");
    exit (1);
}
