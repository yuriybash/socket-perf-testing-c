/* A simple implementation of a server to host a chatroom. The chatroom 
 * consists of up to 10 clients connected at one time and supports both
 * normal messages and the commands found in commands.h. 
 * Author: Nick Riasanovsky */

#ifndef SERVER_H
#define SERVER_H

#include "client_server_utils.h"

#define MAX_NAME_LENGTH 251

#define MAX_CONNECTIONS 11


/* Array of sockets that will be used to send information.
 * A socket will be initialized and reset to -1 if there
 * is not active connection and index 0 will always contain
 * the socket the server uses to receive connections. */
extern fd_t sockets[MAX_CONNECTIONS];

/* Array of messages that have been received from each user but
 * are not yet complete. */
extern char *messages [MAX_CONNECTIONS];

/* Array of user information about users who have connected. */
extern struct user_info *users [MAX_CONNECTIONS];

/* Array of offsets into the messages. */
extern unsigned offsets [MAX_CONNECTIONS];

/* The number have sockets that have currently connected. Includes
 * the servers socket that receives connections. */
extern unsigned socket_total;

/* Function that will handle all connections to the server. It first will
 * initialize the server socket and then transitions into a loop where it will
 * attempt to receive information from its outstanding sockets. */
void handle_connections (int port);

/* Function that handles a new user connecting to the server. It will first
 * accept the new connection and will then place the file descriptor in the
 * socket array. Then it will set its corresponding location in the users
 * array list to NULL and allocate space for it to store messages. */
void establish_connection ();

/* Function that handles the client that is connected with the file descriptor
 * in index n. It will attempt to read information if it exists and will
 * process the information accordingly. If it contains the first full message
 * it will create a user with the name being the message contents. It also
 * handles possible client disconnects and informs other clients of the
 * disconnect. */
void handle_client (unsigned n);

/* Shares a message sent from the user in index n with all other users. */
void share_user_message (char *message, unsigned n);

/* Shares a message to all users except the one located in index
 * n. If isuser then a check for if the user is muted should
 * occur. Should also handle the case where at least 1 of the
 * users disconnected. */
void share_message (char *message, unsigned n, bool isuser);

/* Function to send message to the user located in index n. Should
 * also handle the case in which the user disconnected. */
void reply (char *message, unsigned n);

/* Functiont to handle notifying other users that user n disconnected. */
void handle_disconnect (unsigned n);

#endif
