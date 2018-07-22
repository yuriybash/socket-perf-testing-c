/* File that contains the information necessary to implement all of the
 * supported commands in the server.
 * Author: Yuriy Bash */

#include <stdio.h>
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


/*
 * Function: parse_command
 * -----------------------
 * takes in a message that is a command sent by user at index n (in `users`)
 * and parses the message into a command name and its arguments. It then calls
 * the appropriate function to handle the command.
 *
 * message: the message sent by the user
 *
 * returns: void
 */
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

/*
 * Function: handle_command
 * -----------------------
 * takes a command name, the arguments, the number of arguments, and the index
 * of the user who gave the command and calls the appropriate the appropriate
 * function to handle the command. If the command is a "server_exit",  frees
 * msg.
 *
 * name: the command name
 * args: arguments the command is called with
 * count: the number of arguments
 * n: the index (in `users`) of the user that issues the command
 * msg: the message
 *
 * returns: void
 */
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


/*
 * Function: handle_exit
 * ----------------------
 * handles the exit command. It sends the client back a message to exit using
 * the Exit_Message character (see client_server_utils.h) at the start of the
 * message. The function is also passed in the total number of args the
 * function was called with, count and the index of the user who sent the
 * command.
 *
 * args: arguments the command was called with
 * count: number of arguments the command was called with
 * n: index (in `users`) of connecting client.
 *
 * returns: void
 *
 */
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


/*
 * Function: handle_server_exit
 * ----------------------
 * handles the server_exit  command. It closes all open sockets, frees all the
 * messages for any of the users who were open and cleans up all of the
 * remaining users.
 *
 * args: arguments the command was called with
 * count: number of arguments the command was called with
 * n: index (in `users`) of connecting client.
 *
 * returns: void
 *
 */
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

/*
 * Function: handle_set_nickname
 * ----------------------
 * handles the set_nickname  command. 2 arguments are received, the name of the
 * user referenced and the new nickname. The user referenced must exist, and
 * an error is reported (to the client) if the user does not exist. Once the
 * nickname is set, all messages received by clients from that user must have
 * the new nickname attached to them.
 *
 * args: arguments the command was called with (two args: name, nickname)
 * count: number of arguments the command was called with
 * n: index (in `users`) of connecting client.
 *
 * returns: void
 *
 */
void handle_set_nickname (char **args, unsigned count, unsigned n) {

	struct user_info *user = find_user(args[0]);
	if(user == NULL){
	    reply("Cannot set nickname, user doesn't exist!\n\0", n);
	    return;
	}
    user->nickname = (char **) malloc(sizeof(char *));
	*(user->nickname) = args[1];

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

}

/*
 * Function: handle_clear_nickname
 * ----------------------
 * handles the clear_nickname command. 1 argument is received, the name of the
 * user referenced. The user's nickname is cleared after this is done.
 *
 * args: arguments the command was called with (1 arg: name)
 * count: number of arguments the command was called with
 * n: index (in `users`) of connecting client.
 *
 * returns: void
 *
 */
void handle_clear_nickname (char **args, unsigned count, unsigned n) {

	struct user_info *user = find_user(args[0]);
	user->nickname = &(user->name_info->name);

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

}

/*
 * Function: handle_rename
 * ----------------------
 * handles the rename command. 1 argument is received, the name of the
 * user referenced. The user's name becomes the supplied name after this is
 * done.
 *
 * args: arguments the command was called with (1 arg: the new name)
 * count: number of arguments the command was called with
 * n: index (in `users`) of connecting client.
 *
 * returns: void
 *
 */
void handle_rename (char **args, unsigned count, unsigned n) {
	char *name = args[0];
	struct user_info *user = users[n];
	char *old_name = user->name_info->name;

	user->name_info->name = name;

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

}

/*
 * Function: handle_mute
 * ----------------------
 * handles the handle_mute command. 1 argument is received, the name of the
 * user referenced. If the user is not already muted by the user who called
 * the command, set that user to by muted by for the user who called the
 * command. Then all messages sent by the user who's name is the first
 * argument should not be received by the user who issued the mute command.
 *
 * args: arguments the command was called with (1 arg: the name of the user)
 * count: number of arguments the command was called with
 * n: index (in `users`) of connecting client.
 *
 *  returns: void
 */
void handle_mute (char **args, unsigned count, unsigned n) {
    struct user_info muter = *users[n];

    if (*muter.muted_total >= 11){
        reply("Maximum number of users already muted\n", n);
        return;
    }

    struct user_info *mutee = find_user(args[0]);

    for(int i = 0; i < 11; i++){
        if (muter.muted[i] == mutee->name_info){
            reply("User already muted\n", n);
            return;
        }
    }

    for(int i = 0; i < 11; i++){
        if(muter.muted[i] == NULL){
            muter.muted[i] = mutee->name_info;
            break;
        }
    }

    (*muter.muted_total)++;

	char *messages[3];
	messages[0] = "User ";
	messages[1] = args[0];
	messages[2] = " is now muted\n";
	char *message = create_message (messages, 3);
	reply (message, n);
	free (message);

}

/*
 * Function: handle_unmute
 * ----------------------
 * handles the unmute command. 1 argument is received, the name of the user
 * referenced. It takes one argument, a name, which must be the name of an
 * existing user. If the user is currently muted by the user who called the
 * command. Then that user should resume receiving messages from that user.
 *
 * args: arguments the command was called with (1 arg: the name of the user)
 * count: number of arguments the command was called with
 * n: index (in `users`) of connecting client.
 *
 * returns: void
 */
void handle_unmute (char **args, unsigned count, unsigned n) {

    struct user_info muter = *users[n];

    if(*muter.muted_total < 1){
        reply("User doesn't have anyone muted yet!\n", n);
        return;
    }

    struct user_info *mutee = find_user(args[0]);
    bool mutee_found = false;

    for(int i = 0; i < 11; i++){
        if (muter.muted[i] == mutee->name_info){
            muter.muted[i] = NULL;
            (*muter.muted_total)--;
            mutee_found = true;
        }
    }

    if(!mutee_found){
        reply("User is not muted!\n", n);
        return;
    }

	char *other_messages[3];
	other_messages[0] = "User ";
	other_messages[1] = args[0];
	other_messages[2] = " is no longer muted\n";
	char *message = create_message (other_messages, 3);
	reply (message, n);
	free (message);

}


/*
 * Function: handle_show_status
 * ----------------------
 * handles the show_status command. 1 argument is received, the name of the user
 * referenced. It then displays the status of that user to the user who called
 * the command.
 *
 * args: arguments the command was called with (1 arg: the name of the user)
 * count: number of arguments the command was called with
 * n: index (in `users`) of connecting client.
 *
 * returns: void
 *
 */
void handle_show_status (char **args, unsigned count, unsigned n) {
    struct user_info *user = find_user(args[0]);

    if(user == NULL){
        reply("Sorry, user doesn't exist!\n", n);
        return;
    }
    output_user_status(user, n);
}


/*
 * Function: handle_show_all_statuses
 * ----------------------
 * handles the show_all_statuses command. It returns the status information of
 * all connected users in alphabetical order. It takes no arguments.
 *
 * args: arguments the command was called with (should be 0)
 * count: number of arguments the command was called with
 * n: index (in `users`) of connecting client.
 *
 * returns: void
 *
 */
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


/*
 * Function: handle_invalid_arguments
 * ----------------------------------
 * Called when a command is called with the wrong arguments. It replies to the
 * user at index that the command name was called with the wrong arguments.
 *
 * name: name of user that executed the command
 * n: index (in `users`) of user that executed the command
 *
 * returns: void
 */
void handle_invalid_arguments (char *name, unsigned n) {
        char *start = "Incorrect arguments for ";
        char *end = " command\n";
        char *messages[] = {start, name, end};
        char *new_message = create_message (messages, 3);
        reply (new_message, n);
        free (new_message);
}

/*
 * Function: handle_unknown_command
 * ----------------------------------
 * Called when an unknown command is issued. user at index that the command
 * issued is unknown.
 *
 * name: name of user that executed the command
 * n: index (in `users`) of user that executed the command
 *
 * returns: void
 */
void handle_unknown_command (char *name, unsigned n) {
        char *start = "Unknown command ";
        char *end = "\n";
        char *messages[] = {start, name, end};
        char *new_message = create_message (messages, 3);
        reply (new_message, n);
        free (new_message);
}
