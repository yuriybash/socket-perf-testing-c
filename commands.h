/* File that contains the information necessary to implement all of the
 * commands in the server. 
 * Author: Nick Riasanovsky */

#ifndef COMMANDS_H
#define COMMANDS_H

#define COMMAND_COUNT 9
#define WHITESPACE_SET " \f\n\r\t\v"

extern char *commands[COMMAND_COUNT];

extern void (*command_functions[COMMAND_COUNT]) (char **args, unsigned count, unsigned n);

/* Function that takes in a message that is a command sent by user at index
 * n and parses the message into a command name and its arguments. It then
 * calls the appropriate function to handle a command with that name. */
void parse_command (char *message, unsigned n);

/* A function that takes a command name, the arguments, the number of
 * arguments, and the index of the user who gave the command and calls
 * the appropriate function to handle the command. Has msg so it can
 * free the original pointer on a server exit. */
void handle_command (char *name, char **args, unsigned count, unsigned n, char *msg);

/* Function that handles the exit command. It sends the client back
 * a message to exit using the Exit_Message character (see
 * client_server_utils.h) at the start of the message. */
void handle_exit (char **args, unsigned count, unsigned n);

/* Function that handles the server_exit command. It closes all of
 * the sockets that are currently open, frees all the messages for
 * any of the users who were open and cleans up all of the remaining
 * users. */
void handle_server_exit (char **args, unsigned count, unsigned n);

/* Function that handles the set_nickname command. The command takes
 * exactly 2 arguments. The first is a name which must be the name
 * of an existing user. The second is word which must be a valid
 * choice for a new name. */
void handle_set_nickname (char **args, unsigned count, unsigned n);

/* Function to handle the clear_nickname command. It takes in one
 * argument, a name, which must be the name of an existing user. 
 * The function removes the nickname of the user if one exists. 
 * The function is also passed in the total number of args the 
 * function was called with, count and the index of the user who 
 * sent the command. */
void handle_clear_nickname (char **args, unsigned count, unsigned n);

/* Function to handle the rename command. It takes one argument which must
 * be a valid name. The user who called the command will have their name
 * changed to the name passed in. Then any attempts to access that user
 * will need to refer to this new name and not the old one. Also if the
 * user does not have a nickname this name should be displayed instead
 * of the old name. The function is also passed in the total number of args
 * the function was called with, count and the index of the user who sent
 * the command. */
void handle_rename (char **args, unsigned count, unsigned n);

/* Function to handle the mute command. It takes one argument, a name, which
 * must be the name of an existing user. If the user is not already muted
 * by the user who called the command, set that user to by muted by for the
 * user who called the command. Then all messages sent by the user who's name
 * is the first argument should not be received by the user who issued the
 * mute command. The function is also passed in the total number of args the
 * function was called with, count and the index of the user who sent the
 * command. */
void handle_mute (char **args, unsigned count, unsigned n);

/* Function to handle the unmute command. It takes one argument, a name, which
 * must be the name of an existing user. If the user is currently muted by the
 * user who called the command. Then that user should resume receiving messages
 * from that user. The function is also passed in the total number of args the
 * function was called with, count and the index of the user who sent the
 * command. */
void handle_unmute (char **args, unsigned count, unsigned n);

/* Function that handles the show_status command. It takes 1 argument,
 * a name, which must be an existing user. It then display the status
 * of that user to the user who called the command. The function is
 * also passed in the total number of args the function was called with,
 * count and the index of the user who sent the command. */
void handle_show_status (char **args, unsigned count, unsigned n);

/* Function that handles the show_all_statuses command. It returns the status
 * information of all connected users in alphabetical order. It takes no arguments.
 * The function is also passed in the total number of args the function was called
 * with, count and the index of the user who sent the command. */
void handle_show_all_statuses (char **args, unsigned count, unsigned n);

/* Function that handles a known command being called with the wrong
 * arguments. It replies to the user at index that the command name
 * was called with the wrong arguments. */
void handle_invalid_arguments (char *name, unsigned n);

/* Function that handles a command that is not known. It replies to
 * the user at index n that the command name does not exist. */
void handle_unknown_command (char *name, unsigned n);

#endif
