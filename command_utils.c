/* File which contains functions to assist implementation of the
 * various commands. 
 * Author: Nick Riasanovsky */

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "server.h"
#include "command_utils.h"
#include "server_utils.h"
#include "commands.h"
#include "server_utils.h"

/* Function to determine if a message is a command. */
bool iscommand (char *message) {
        int i = 0;
        while (isspace (message[i]) && message[i] != 0) {
                i++;
        }
        return message[i] == '\\' && (message[i] != 0 && (isidentifierpart (message[i + 1])));
}

/* Function to determine if a name is the name of a command. */
bool isknowncommand (char *name) {
        for (int i = 0; i < COMMAND_COUNT; i++) {
                if (strcmp (name, commands[i]) == 0) {
                        return true;
                }
        }
        return false;
}

/* Function that takes in a name and determines if it can be selected as a name
 * for a nickname or a name. */
bool isvalidname (char *name) {
        return isword (name) && strlen (name) <= MAX_NAME_LENGTH && !istaken_name (name) && !istaken_nickname (name);
}

/* Function that sorts the Users based upon alphabetical order of names. This 
 * function is called in coordination with the show_all_statuses command. Since
 * this is called by a specific user in location n, n needs to be updated to
 * ensure the correct user receives the message. */
void sort_users (unsigned *n) {
        remove_holes (n);
        int lower = 1;
        int upper = socket_total - 1;
        quick_sort (lower, upper, n);
}

/* A simple quicksort implementation to assist sorting users. */
void quick_sort (int lower, int upper, unsigned *n) {
	if (lower >= upper) {
                return;
        }
        srand ((unsigned) time (NULL));
        int pivot = lower + (rand () % (upper - lower));
        swap_information (pivot, lower, n);
        pivot = lower;
        unsigned i = lower + 1;
        while (i < upper + 1) {
                if (strcmp (users[i]->name_info->name, users[pivot]->name_info->name) < 0) {
                        swap_information (pivot, i, n);
                        pivot++;
                        swap_information (pivot, i, n);
                        if (pivot == i) {
                                i++;
                        }
                } else {
                        i++;
                }
        }
        quick_sort (lower, pivot - 1, n);
        quick_sort (pivot + 1, upper, n);
}

/* Function to update the array of users by removing any gaps that developed
 * from users disconnecting. */
void remove_holes (unsigned *n) {
        int start = 1;
        int ctr;
        while (start < socket_total) {
                ctr = start + 1;
                while (sockets[start] == -1) {
                        if (sockets[ctr] == -1) {
                                ctr++;
                        } else {
                                swap_information (start, ctr, n);
                        }
                }
                start++;
        }
}

/* A swap function to swap all of the information about two
 * users. This will allow for sorting users. */
void swap_information (int a, int b, unsigned *n) {
        int fd_temp = sockets[b];
        sockets[b] = sockets[a];
        sockets[a] = fd_temp;
        char *message_temp = messages[b];
        messages[b] = messages[a];
        messages[a] = message_temp;
        unsigned offset_temp = offsets[b];
        offsets[b] = offsets[a];
        offsets[a] = offset_temp;
        struct user_info *user_temp = users[b];
        users[b] = users[a];
        users[a] = user_temp;
        if (a == *n) {
                *n = b;
        } else if (b == *n) {
                *n = a;
        }
}

/* Helper function to output the information about user to 
 * the user in socket location n. This should be used for
 * the show_status and show_all_statuses commands. */
void output_user_status (struct user_info *user, unsigned n) {
        char *messages[3];
        messages[0] = "User ";
        messages[1] = user->name_info->name;
        messages[2] = "\n";
        char *message1 = create_message (messages, 3);
        if (sockets[n] != -1 && has_nickname (user)) {
                messages[0] = "Nickname ";
                messages[1] = *(user->nickname);
                messages[2] = "\n";
        } else {
                messages[0] = "User has no ";
                messages[1] = "nickname";
                messages[2] = "\n";
        }
        char *message2 = create_message (messages, 3);
        if (sockets[n] != -1 && ismuted (users[n], user)) {
                messages[0] = "User is muted\n";
        } else {
                messages[0] = "User is not muted\n";
        }
        char *message3 = create_message (messages, 1);
	messages[0] = message1 + 1;
	messages[1] = message2;
	messages[2] = message3;
	char *message = create_message (messages, 3);
	free (message1);
	free (message2);
	free (message3);
        reply (message, n);
        free (message);
}

