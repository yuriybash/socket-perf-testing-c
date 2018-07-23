/* A file of functions involving processing messages for students to implement.
 * Author: Yuriy Bash */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "client.h"
#include "client_utils.h"
#include "client_server_utils.h"
#include "student_client.h"

/* Buffer to hold characters being read in from stdin. */
char input_message[MAX_INPUT_LENGTH + 1];

/* Variable that holds the next available spot in input_message. */
unsigned input_offset;


/*
 * Function: process_input
 * -----------------------
 *
 * Processes input from stdin to be written to the server using send_message.
 *
 * If only part of a message has been written, the information is stored in
 * input_message at the input_offset index. Messages are terminated with '\n'
 * _and_ '\0'.
 *
 * returns: void
 */
void process_input () {
     int c;
     input_offset = 0;

     while((c = getc(stdin)) != '\n'){
         input_message[input_offset] = c;
         input_offset++;
     }

     input_message[input_offset] = '\n';
     input_message[input_offset+1] = '\0';
     send_message();
     display_prefix();

     memset(input_message, 0, sizeof(input_message));


}


/*
 * Function: handle_server_message
 * -----------------------
 *
 * Function that takes in a whole message from a server and handles it
 * appropriately. A message from a server will have a leading byte that
 * indicates what time of message it is (either Standard_Message or
 * Exit_Message, see the enum in client_server_utils.h). If exit, handle_exit
 * is called, otherwise the message is output (without leading byte).
 *
 * msg: an entire message from the server
 *
 * returns: void
 */
void handle_server_message (char *msg) {
        clear_prefix ();
        if (msg[0] == Exit_Message) {
		free (msg);
                handle_exit ();
        } else {
            clear_prefix();
		    printf("%s", (msg));
            display_prefix();
        }
}

/*
 * Function: handle_exit
 * -----------------------
 *
 * Handles a response from the server telling it to exit. This is the result
 * of the server determining the user has properly issued the \exit command.
 *
 * returns: void
 */
void handle_exit () {
	printf ("You have left\n");
	close (socket_fd);
	exit (0);
}
