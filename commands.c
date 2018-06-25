/* File that contains the information necessary to implement all of the
 * commands in the server.
 * Author: YOUR NAME HERE */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include "server.h"
#include "commands.h"
#include "command_utils.h"
#include "server_utils.h"
#include "user_utils.h"
#include "client_server_utils.h"

/* List of commands the server recognizes. */
char* commands[COMMAND_COUNT] = {"exit", "server_exit", "set_nickname", "clear_nickname",
"rename", "mute", "unmute", "show_status", "show_all_statuses"};

/* List of functions to handle each command. Each function is at the same index
 * as the command is in the previous list to allow a command name search to
 * give easy access to a function with an extended if else. */
void (*command_functions[COMMAND_COUNT]) (char **args, unsigned count, unsigned n) =
{handle_exit, handle_server_exit, handle_set_nickname, handle_clear_nickname, handle_rename,
handle_mute, handle_unmute, handle_show_status, handle_show_all_statuses};


/* Function that takes in a message that is a command sent by user at index
 * n and parses the message into a command name and its arguments. It then
 * calls the appropriate function to handle a command with that name. */
void parse_command (char *message, unsigned n) {
        int i = 0;
        while (message[i] != '\\') {
                i++;
        }
        char *name = strtok (message + i + 1, WHITESPACE_SET);
        if (!isword (name)) {
                int end = 0;
                while (isidentifierpart (name[end])) {
                        end++;
                }
                name[end] = 0;
                if (isknowncommand (name)) {
                        handle_invalid_arguments (name, n);
                        return;

                } else {
                        handle_unknown_command (name, n);
                        return;
                }
        }
        if (!isknowncommand (name)) {
                handle_unknown_command (name, n);
        } else {
                unsigned arg_limit = 3;
                char *args[3];
                char *arg;
                unsigned count = 0;
                while (count < arg_limit && (arg = strtok (NULL, WHITESPACE_SET)) != NULL) {
                        args[count] = arg;
                        count++;
                        if (!isword (arg)) {
                                handle_invalid_arguments (name, n);
                                return;
                        }
                }
                if (count == arg_limit) {
                        handle_invalid_arguments (name, n);
                        return;
                }
                handle_command (name, args, count, n, message);
        }

}

/* A function that takes a command name, the arguments, the number of
 * arguments, and the index of the user who gave the command and calls
 * the appropriate function to handle the command. Needs to also have 
 * a ptr to original msg to free if it is a server_exit. */
void handle_command (char *name, char **args, unsigned count, unsigned n, char *msg) {
        unsigned ctr = 0;
        while (ctr < COMMAND_COUNT) {
                if (strcmp (name, commands[ctr]) == 0) {
			if (strcmp (name, "server_exit") == 0 && count == 0) {
                               free (msg);
                        }
                        command_functions [ctr] (args, count, n);
                        return;
                }
                ctr++;
        }
        handle_unknown_command (name, n);

}

/* Function that handles the exit command. It sends the client back
 * a message to exit using the Exit_Message character (see
 * client_server_utils.h) at the start of the message. The
 * function takes in no arguments. The function is also passed in
 * the total number of args the function was called with, count and
 * the index of the user who sent the command. */
void handle_exit (char **args, unsigned count, unsigned n) {
        if (count != 0) {
                handle_invalid_arguments ("exit", n);
        } else {
                char message [3];
                message[0] = Exit_Message;
                message[1] = '\n';
                message[2] = 0;
                reply (message, n);
        }
}

/* Function that handles the server_exit command. It closes all of
 * the sockets that are currently open, frees all the messages for
 * any of the users who were open and cleans up all of the remaining
 * users. The function takes in no arguments. The function is also
 * passed in the total number of args the function was called with,
 * count and the index of the user who sent the command. */
void handle_server_exit (char **args, unsigned count, unsigned n) {
        if (count != 0) {
                handle_invalid_arguments ("server_exit", n);
        } else {
                close (sockets[0]);
                socket_total--;
		free (messages[0]);
                int start = 0;
                int ctr = 1;
                while (start < socket_total) {
                        if (sockets[ctr] != -1) {
                                start++;
                                close (sockets[ctr]);
                                sockets[ctr] = -1;
                                free (messages[ctr]);
                                if (users[ctr] != NULL) {
                                        cleanup_user (users[ctr]);
                                }
                        }
                        ctr++;
                }
                exit (0);
        }
}






 /* ***************************READ ME*******************************
  * 								    *
  *								    *
  * This begins the section you will edit. If you encounter an error*
  * be sure and call the handle_invalid_arguments function and then *
  * exit the function. See handle_server_exit above for an example  *
  * of how to use it. Also be sure and check the error conditions   *
  * on the spec.                                                    *
  * 					                            *
  * A few functions you may find userful:                           *
  *       - isvalidname (command_utils.h) 			    *
  *       - find_user (user_utils.h)                                *
  *       - has_nickname (user_utils.h)                             *
  *       - create_name (user_utils.h)                              *
  *       - output_user_status (command_utils.h)                    *
  *                                                                 *
  * ****************************************************************/



/* Function that handles the set_nickname command. The command takes
 * exactly 2 arguments, stored in args. The first is a name which
 * must be the name of an existing user. The second is word which
 * must be a valid choice for a new name. It sets the user whose name
 * is given by the first argument's nickname to the second argument.
 * Then all messages from that user should display that nickname.
 * The function is also passed in the total number of args the function
 * was called with, count and the index of the user who sent the command.
 * You do not need to check that any args passed in are either too long 
 * or consist of invalid characters, this has already been checked for you.*/
void handle_set_nickname (char **args, unsigned count, unsigned n) {
	/* HANDLE ANY POSSIBLE ERROR CONDITIONS */


	/* IMPLEMENT THE CORE FUNCTIONALITY */

	struct user_info *user = NULL; /* REPLACE ME */

	/* WE HANDLE MESSAGE OUTPUT FOR YOU */
	char *other_messages[6];
	other_messages[0] = users[n]->name_info->name;
	other_messages[1] = " set ";
	other_messages[2] = user->name_info->name;
	other_messages[3] = "'s nickname to ";
	other_messages[4] = args[1];
	other_messages[5] = "\n";
	char *message = create_message (other_messages, 6);
	share_message (message, n, false);
	free (message);
	other_messages[0] = "You set ";
	if (user == users[n]) {
		other_messages[1] = "your";
		other_messages[2] = " nickname to ";
        } else {
                other_messages[1] = other_messages[2];
		other_messages[2] = other_messages[3];
	}
	other_messages[3] = other_messages[4];
	other_messages[4] = other_messages[5];
	message = create_message (other_messages, 5);
	reply (message, n);
	free (message);

	/* ANYTHING ELSE THAT NEEDS TO BE DONE? DOES ANYTHING
	 * NEED TO BE FREED? */
}

/* Function to handle the clear_nickname command. It takes in one
 * argument, a name, which must be the name of an existing user.
 * The function removes the nickname of the user if one exists
 * and resets it to that user's name.
 * The function is also passed in the total number of args the
 * function was called with, count and the index of the user who
 * sent the command. You do not need to check that any args passed 
 * in are either too long or consist of invalid characters, this 
 * has already been checked for you. */
void handle_clear_nickname (char **args, unsigned count, unsigned n) {
	/* HANDLE ANY POSSIBLE ERROR CONDITIONS */


	/* IMPLEMENT THE CORE FUNCTIONALITY */

	struct user_info *user = NULL; /* REPLACE ME */

	/* WE HANDLE MESSAGE OUTPUT FOR YOU */
	char *other_messages[4];
	other_messages[0] = users[n]->name_info->name;
	other_messages[1] = " has cleared ";
	other_messages[2] = user->name_info->name;
	other_messages[3] = "'s nickname\n";
	char *message = create_message (other_messages, 4);
	share_message (message, n, false);
	free (message);
	other_messages[0] = "You have cleared ";
	if (user == users[n]) {
		other_messages[1] = "your";
		other_messages[2] = " nickname\n";
	} else {
		other_messages[1] = other_messages[2];
		other_messages[2] = other_messages[3];
	}
	message = create_message (other_messages, 3);
	reply (message, n);
	free (message);

	/* ANYTHING ELSE THAT NEEDS TO BE DONE? DOES ANYTHING
	 * NEED TO BE FREED? */
}

/* Function to handle the rename command. It takes one argument which must
 * be a valid name. The user who called the command will have their name
 * changed to the name passed in. Then any attempts to access that user
 * will need to refer to this new name and not the old one. Also if the
 * user does not have a nickname this name should be displayed instead
 * of the old name. The function is also passed in the total number of args
 * the function was called with, count and the index of the user who sent
 * the command. You do not need to check that any args passed in are either
 * too long or consist of invalid characters, this has already been checked 
 * for you.*/
void handle_rename (char **args, unsigned count, unsigned n) {
	/* HANDLE ANY POSSIBLE ERROR CONDITIONS */

	/* USEFUL VARIABLES PRESET FOR MESSAGE OUTPUT */
	char *name = args[0];
	struct user_info *user = users[n];
	char *old_name = user->name_info->name;


	/* IMPLEMENT THE CORE FUCNTIONALITY */


	/* WE HANDLE MESSAGE OUTPUT FOR YOU */
	char *other_messages[4];
	other_messages[0] = old_name;
	other_messages[1] =" changed their name to ";
	other_messages[2] = name;
	other_messages[3] = "\n";
	char *message = create_message (other_messages, 4);
	share_message (message, n, false);
	free (message);
	other_messages[0] = "You have changed your name to ";
	other_messages[1] = name;
	other_messages[2] = "\n";
	message = create_message (other_messages, 3);
	reply (message, n);
	free (message);

        /* ANYTHING ELSE THAT NEEDS TO BE DONE? DOES ANYTHING
	 * NEED TO BE FREED? */
}

/* Function to handle the mute command. It takes one argument, a name, which
 * must be the name of an existing user. If the user is not already muted
 * by the user who called the command, set that user to by muted by for the
 * user who called the command. Then all messages sent by the user who's name
 * is the first argument should not be received by the user who issued the
 * mute command. The function is also passed in the total number of args the
 * function was called with, count and the index of the user who sent the
 * command. */
void handle_mute (char **args, unsigned count, unsigned n) {
	/* HANDLE ERROR CONDITIONS */

	/* IMPLEMENT THE CORE FUNCTIONALITY */


	/* WE HANDLE MESSAGE OUTPUT FOR YOU */
	char *messages[3];
	messages[0] = "User ";
	messages[1] = args[0];
	messages[2] = " is now muted\n";
	char *message = create_message (messages, 3);
	reply (message, n);
	free (message);

	/* ANYTHING ELSE THAT NEEDS TO BE DONE? DOES ANYTHING
	 * NEED TO BE FREED? */
}

/* Function to handle the unmute command. It takes one argument, a name, which
 * must be the name of an existing user. If the user is currently muted by the
 * user who called the command. Then that user should resume receiving messages
 * from that user. The function is also passed in the total number of args the
 * function was called with, count and the index of the user who sent the
 * command. You do not need to check  that any args passed in are either too long
 * or consist of invalid characters, this has already been checked for you.*/
void handle_unmute (char **args, unsigned count, unsigned n) {
	/* HANDLE ERROR CONDITIONS */

	/* IMPLEMENT THE CORE FUNCTIONALITY */


	/* WE HANDLE MESSAGE OUTPUT FOR YOU */
	char *other_messages[3];
	other_messages[0] = "User ";
	other_messages[1] = args[0];
	other_messages[2] = " is no longer muted\n";
	char *message = create_message (other_messages, 3);
	reply (message, n);
	free (message);

	/* ANYTHING ELSE THAT NEEDS TO BE DONE? DOES ANYTHING
	 * NEED TO BE FREED? */
}

/* Function that handles the show_status command. It takes 1 argument,
 * a name, which must be an existing user. It then display the status
 * of that user to the user who called the command. The function is
 * also passed in the total number of args the function was called with,
 * count and the index of the user who sent the command. You may find
 * the function output_user_status helpful. You do not need to check
 * that any args passed in are either too long or consist of invalid
 * characters, this has already been checked for you. */
void handle_show_status (char **args, unsigned count, unsigned n) {
        /* HANDLE ERROR CONDITIONS. */

	/* IMPLEMENT THE CORE FUNCTIONALITY */
}

/* Function that handles the show_all_statuses command. It returns the status
 * information of all connected users in alphabetical order. It takes no arguments.
 * The function is also passed in the total number of args the function was called
 * with, count and the index of the user who sent the command. You may find the
 * functions sort_users and output_user_status (in command_utils.h)
 * helpful. You do not need to check that any args passed in are either too long 
 * or consist of invalid characters, this has already been checked for you. */
void handle_show_all_statuses (char **args, unsigned count, unsigned n) {
	if (count != 0) {
                handle_invalid_arguments ("show_all_statuses", n);
        } else {
                sort_users(&n);
                for (int i = 1; i < socket_total; i++) {
                        output_user_status (users[i], n);
                }
        }
}

/* Function that handles a known command being called with the wrong
 * arguments. It replies to the user at index that the command name
 * was called with the wrong arguments. */
void handle_invalid_arguments (char *name, unsigned n) {
        char *start = "Incorrect arguments for ";
        char *end = " command\n";
        char *messages[] = {start, name, end};
        char *new_message = create_message (messages, 3);
        reply (new_message, n);
        free (new_message);
}

/* Function that handles a command that is not known. It replies to
 * the user at index n that the command name does not exist. */
void handle_unknown_command (char *name, unsigned n) {
        char *start = "Unknown command ";
        char *end = "\n";
        char *messages[] = {start, name, end};
        char *new_message = create_message (messages, 3);
        reply (new_message, n);
        free (new_message);
}
