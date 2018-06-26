/* A simple implementation of a server to host a chatroom. The chatroom 
 * consists of up to 10 clients connected at one time and supports both
 * normal messages and the commands found in commands.h. 
 * Author: Nick Riasanovsky */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "server.h"
#include "commands.h"
#include "command_utils.h"
#include "user_utils.h"
#include "server_utils.h"
#include "client_server_utils.h"

void socket_error ();

/* Array of sockets that will be used to send information. 
 * A socket will be initialized and reset to -1 if there
 * is not active connection and index 0 will always contain
 * the socket the server uses to receive connections. */
fd_t sockets[11];

/* Array of messages that have been received from each user but
 * are not yet complete. */
char *messages [11];

/* Array of user information about users who have connected. */
struct user_info *users [11];

/* Array of offsets into the messages. */
unsigned offsets [11];

/* The number have sockets that have currently connected. Includes
 * the servers socket that receives connections. */
unsigned socket_total;

/* This is a simple chat server which will host up to 10 clients to communicate
 * in a single location. */
int main (int argc, char *argv[]) {
	if (argc != 2) {
		fprintf (stderr, "Server takes exactly 1 argument, the port\n");
		exit (1);
	}
	int port = atoi (argv[1]);
	handle_connections (port);
}

/* Function that will handle all connections to the server. It first will
 * initialize the server socket and then transitions into a loop where it will
 * attempt to receive information from its outstanding sockets. */
void handle_connections (int port) {
	fd_t main_socket = socket (AF_INET, SOCK_STREAM, 0);
	if (main_socket == -1) {
		socket_error ();
	}
	int flags = fcntl (main_socket, F_GETFL, 0);
	if (flags == -1) {
		socket_error ();
	}
	int err = fcntl (main_socket, F_SETFL, flags | O_NONBLOCK);
	if (err == -1) {
		socket_error ();
	}
	struct sockaddr_in info;
	info.sin_family = AF_INET;
	info.sin_port = htons (port);
	info.sin_addr.s_addr = htonl (INADDR_ANY);
	if (bind (main_socket, (struct sockaddr *) &info, sizeof (info)) == -1) {
		socket_error ();
	}
	if (listen (main_socket, 10) == -1) {
		socket_error ();
	}
	memset (offsets, 0, sizeof (unsigned) * 11);
	messages[0] = malloc (sizeof (char) * (MAX_MESSAGE_LENGTH + 1));
	if (messages[0] == NULL) {
		allocation_failed ();
	}
	memset (sockets + 1, -1, sizeof (fd_t) * 10);
	sockets[0] = main_socket;	
	socket_total = 1;
	fd_set read_set;
	fd_set except_set;
	printf ("Server Messages:");
	fflush (stdout);
	while (1) {
		FD_ZERO (&read_set);
		FD_ZERO (&except_set);
		fd_t socket = 0;
		unsigned fd_max = 0;
		unsigned count = 0;
		unsigned n = 0;
		while (count < socket_total) {
			if ((socket = sockets[n]) != -1) {
				count++;
				FD_SET (socket, &read_set);
				FD_SET (socket, &except_set);
				if (socket > fd_max) {
					fd_max = socket;
				}
			}
			n++;
		}
		n = 1;
		count = 1;
		unsigned temp = 0;
		select (fd_max + 1, &read_set, NULL, &except_set, NULL);
		while (count < socket_total) {
			if (sockets[n] != -1) {
				count++;
				if (FD_ISSET (sockets[n], &except_set)) {
					close (sockets[n]);
					sockets[n] = -1;
					temp++;
					free (messages[n]);
					messages[n] = NULL;
					if (users[n] != NULL) {
						handle_disconnect (n);
						cleanup_user (users[n]);
					}
				}
			}
			n++;
		}
		socket_total -= temp;
		n = 0;
		count = 0;
		while (count < socket_total) {
			if (sockets[n] != -1) {
				count++;
				if (FD_ISSET (sockets[n], &read_set)) {
					if (n == 0) {
						if (socket_total < 11) {
							establish_connection ();
						}
					} else {
						handle_client (n);
					}
				}
			}
			n++;
		}
	}
}

/* Function that handles a new user connecting to the server. It will first
 * accept the new connection and will then place the file descriptor in the
 * socket array. Then it will set its corresponding location in the users
 * array list to NULL and allocate space for it to store messages. */
void establish_connection () {
	struct sockaddr_in info;
	unsigned size = sizeof (info);
	fd_t new_fd = accept (sockets[0], (struct sockaddr *) &info, &size);
	if (new_fd > 0) {
		socket_total++;
		bool success = false;
		unsigned counter = 1;
		while (!success) {
			if (sockets[counter] == -1) {
				success = true;
				sockets[counter] = new_fd;
				messages[counter] = malloc (sizeof (char) * (MAX_MESSAGE_LENGTH + 1));
				if (messages[counter] == NULL) {
					allocation_failed ();
				}
				users[counter] = NULL;
				int flags = fcntl (new_fd, F_GETFL, 0);
				if (flags == -1) {
					socket_error ();
				}
				flags = fcntl (new_fd, F_SETFL, flags | O_NONBLOCK);
				if (flags == -1) {
					socket_error ();
				}
			} else {
				counter++;
			}
		}
	}
}

/* Function that handles the client that is connected with the file descriptor
 * in index n. It will attempt to read information if it exists and will 
 * process the information accordingly. If it contains the first full message
 * it will create a user with the name being the message contents. It also
 * handles possible client disconnects and informs other clients of the
 * disconnect. */
void handle_client (unsigned n) {
	errno = 0;
	int length = read (sockets[n], messages[n] + offsets[n], MAX_MESSAGE_LENGTH - offsets[n]);
	int location;
	if (length > 0) {
		messages[n][length + offsets[n]] = 0;
		while ((location = find_message_end (messages[n], offsets[n])) != -1) {
			char *message = generate_message (messages[n], location + 1);
			if (users[n] == NULL) {
				message [strlen(message) - 1] = 0;
				users[n] = create_user (message);
				char* message_parts[2];
				message_parts[0] = message;
				message_parts[1] = " has joined\n";
				printf ("%s%s", message_parts[0], message_parts[1]);
				fflush (stdout);
				char *entry_message = create_message (message_parts, 2);
				share_message (entry_message, n, false);
				free (entry_message);
			} else {
				printf ("%s", message);
				fflush (stdout);
				if (iscommand (message)) {
					parse_command (message, n);
				} else {
					share_user_message (message, n);
				}
			}
			free (message);
			offsets[n] = 0;
		}
		offsets[n] = strlen (messages[n]);
	} else if (length == 0 || (errno && errno != EAGAIN && errno != EWOULDBLOCK)) {
		close (sockets[n]);
		sockets[n] = -1;
		socket_total--;
		free (messages[n]);
		messages[n] = NULL;
		if (users[n] != NULL) {
			handle_disconnect (n);
			cleanup_user (users[n]);
		}
	}
}

/* Shares a message sent from the user in index n with all other users. */
void share_user_message (char *message, unsigned n) {
	char *messages[3];
	messages[0] = *(users[n]->nickname);
	messages[1] = ":";
	messages[2] = message;
	char *new_message = create_message (messages, 3);
	share_message (new_message, n, true);
	free (new_message);
}

/* Shares a message to all users except the one located in index
 * n. If isuser then a check for if the user is muted should
 * occur. Should also handle the case where at least 1 of the
 * users disconnected. */
void share_message (char *message, unsigned n, bool isuser) {
	errno = 0;
	int length;
	int ctr = 1;
	int count = 1;
	int size;
	int total_length = strlen (message);
	unsigned closures = 0;
	unsigned closure_list[socket_total];
	while (count < socket_total) {
		if (sockets[ctr] != -1) {
			count++;
			if (ctr != n) {
				if (!isuser ||(users[n] != NULL && !ismuted (users[ctr], users[n]))) {
					length = 0;
					while (length < total_length) {
						size = write (sockets[ctr], message + length, total_length - length);
						if (size == 0 || (errno && errno != EAGAIN && errno != EWOULDBLOCK)) {
							close (sockets[ctr]);
							sockets[ctr] = -1;
							closure_list [closures] = ctr;
							closures++;
							break;
						} else {
							length += size;
						}
					}
				}	
			}
		}
		ctr++;	
	}
	socket_total -= closures;
	for (int i = 0; i < closures; i++) {
		free (messages[n]);
		messages[n] = NULL;
		messages[closure_list [i]] = NULL;
		if (users[closure_list [i]] != NULL) {
			handle_disconnect (closure_list [i]);
			cleanup_user (users[closure_list [i]]);
		}
	}
}

/* Function to send message to the user located in index n. Should
 * also handle the case in which the user disconnected. */
void reply (char *message, unsigned n) {
	if (sockets[n] == -1) {
		return;
	}
	int size;
	int length = 0;
	int total_length = strlen (message);
	while (length < total_length) {
		size = write (sockets[n], message + length, total_length - length);
		if (size == -1) {
			close (sockets[n]);
			free (messages[n]);
			messages[n] = NULL;
			sockets[n] = -1;
			socket_total--;
			if (users[n] != NULL) {
				handle_disconnect (n);
				cleanup_user (users[n]);
			}
			return;
		}
		length += size;
	}
}

/* Functiont to handle notifying other users that user n disconnected. */
void handle_disconnect (unsigned n) {
	char *messages[2];
	messages[0] = users[n]->name_info->name;
	messages[1] = " has left\n";
	char *message = create_message (messages, 2);
	share_message (message, n, false);
	free (message);
}

/* Function to handle an error that occurs when setting up the server. */
void socket_error () {
	fprintf (stderr, "Unable to create server socket\n");
	exit (1);
}
