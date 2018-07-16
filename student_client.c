/* A file of functions involving processing messages for students to implement.
 * Author: YOUR NAME HERE */

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

 /* Processes input from stdin to be written to the server using send_message.
 * If only part of a message has been written then the information should be
 * stored in the input_message array and the next available location stored at
 * input_offset. In particular we will assume that a complete message will
 * never exceed MAX_INPUT_LENGTH (a macro defined in client_server_utils.h
 * which includes the space for the '\n' and null termininator) and all 
 * complete messages will be terminated with a newline character. To do 
 * this you will likely want to use the built in getc function. One 
 * important note about the getc function is that since we are reading from 
 * stdin while it is non-blocking (meaning we do not wait for input to arrive)
 * if no input is available getc will return the MACRO EOF, which has the 
 * value -1. You will need to explicitly check for this as otherwise our 
 * autograder my note be able to have your client receive messages. Finally, 
 * don't forget to readd the "[Me:]" prefix after the message is sent. 
 * You may find a display_prefix helpful. To send a message you will want
 * to use the send_message function that is provided for you in client_utils.h.
 * Don't forget to null terminate any string you send. */
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

/* Function that takes in a whole message from a server and handles
 * it appropriately. A message from a server will have a leading byte that
 * indicates what type of message it is. This will either be a
 * Standard_Message or an Exit_Message (see the enum in client_server_utils.h).
 * If the message is an Exit_Message the user should call the 
 * handle_exit function to exit appropriately. Otherwise the message should 
 * be output (without the message type byte). Be sure and remove and readd 
 * the "[Me:]" prefix. You may find the clear_prefix and display_prefix helper
 * functions helpful. */
void handle_server_message (char *msg) {
        clear_prefix ();
        if (msg[0] == Exit_Message) {
		free (msg);
                handle_exit ();
        } else {
            display_prefix();
		    printf("%s", (msg+1));
        }
}

/* Handles a response from the server telling it to exit. This is the result
 * of the server determining the user has properly issued the \exit command. 
 * You will need to call the c library close function on socket_fd as part
 * of a proper exit. */
void handle_exit () {
	printf ("You have left\n");
	close (socket_fd);
	exit (0);
}
