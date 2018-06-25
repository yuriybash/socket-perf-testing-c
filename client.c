/* This is a simple implementation of a client that connects to a server
 * in order to create a "chatroom". This model is fairly simplistic because
 * there exists only one room for everyone to particpate in. To start an
 * instance of a client a user must provide 3 command line arguments when
 * executing the program. The first is the name of the user, which is assumed
 * to be unique among the users. You need not handle the case where a user
 * with the same name already exists in the room when you join.
 * The second command line argument will be the IP v4 address at which the server 
 * can be found. This should be supplied in the form a.b.c.d where a, b, c, and d 
 * are all integers between 0 and 255. If you wish the run the server on your local
 * machine you can use the address 127.0.0.1. In fact you will find that
 * as a result of NATs, the solution to address depreciation problem in IP v4
 * you cannot host a server using many computers as they will have a shared IP
 * address. Lastly you need to supply a port number at which to find the server. 
 * This should be shared with the value the server is called. Since most low port
 * numbers are reserved you will want to choose a large random number in the
 * neightborhood of say 10000. 
 * Author: Nick Riasanovsky*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "client.h"
#include "student_client.h"
#include "client_utils.h"
#include "client_server_utils.h"

/* File descriptor for the socket that connects to the server. */
fd_t socket_fd;

/* Buffer to hold the messages being received from the server. */ 
char server_message [MAX_MESSAGE_LENGTH];

/* Variable to hold next available location in server_message. */
unsigned server_offset;

int main (int argc, char *argv[]) {
	/* Note argument 0 to argv is always the path of the program. As a 
	 * result we want to verify that argc is 4. */
	if (argc != 4) {
		fprintf (stderr, "Wrong number of command line arguments. " \
			"This program is called with:\n" \
			"./EXECUTABLE_NAME username ip_address port\n");
		exit (1);
	}
	char *username = argv[1];
	char *hostname = argv[2];
	short port = atoi (argv[3]);

	/* Now that the username, hostname, and port are resolved
	 * we can create a socket connection with the server. */
	establish_connection (username, hostname, port);


	/* Set stdin to be nonblocking. This will allow us to switch between checking
	 * stdin and checking the server for data. */
	setup_input ();

	/* Program should now cycle between receiving data from the server and receiving
	 * data from the user (and then sending the data) until a command to end the
	 * connection is given. */
	maintain_connection ();
}



/* Takes in a username, hostname, and port and establishes a connection to
 * a server. The program will first connect to the server and then send
 * the username followed by a newline to the server to allow the server
 * to initialize user information. Then the socket will be set to nonblocking
 * to allow for cycling between it and stdin. If at any point the action is
 * not successful the client will exit. */
 void establish_connection (char *username, char *hostname, short port) {
	socket_fd = socket (AF_INET, SOCK_STREAM, 0);
	if (socket_fd == -1) {
		fprintf (stderr, "Unable to create socket\n");
		exit (1);
	} 
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;

	/* Need to transition port from little endian to big endian. */
	addr.sin_port = htons (port);

	int err =  inet_pton (AF_INET, hostname, &addr.sin_addr);
	if (err != 1) {
		fprintf (stderr, "Address is not a valid IPv4 address\n");
		exit (1);
	}
	int success = connect (socket_fd, (struct sockaddr *) &addr, sizeof(addr));
	if (success == -1) {
		fprintf (stderr, "Unable to connect to the server\n");
		exit (1);
	}
	int size;
	int total_length = strlen (username);
	int length = 0;
	/* Write may not be able to write the entire length at once so how much
	 * succeeds needs to be checked. */
	while (length < total_length) {
		size = write (socket_fd, username + length, total_length - length);
		/* 0 means the server terminated the connection and -1 indicates an
		 * error. Note exiting may not be completely successful if the server
		 * crashes during before the write finishes. */
		if (size <= 0) {
			handle_server_disconnect ();
		}
		length += size;
	}
	err = write (socket_fd, "\n", 1);
	if (err <= 0) {
		handle_server_disconnect ();
	}
	/* Need to extract the existing file descriptor flags. */
        int flags = fcntl (socket_fd, F_GETFL, 0);
        if (flags == -1) {
                fprintf (stderr, "Unable to set socket to nonblocking\n");
                close (socket_fd);
                exit (1);
        }
        /* Every flag is just a single bit so using the bitwise or
         * opperation on the flags with the nonblocking flag will
         * set the file descriptor to nonblocking. */
        err = fcntl (socket_fd, F_SETFL, flags | O_NONBLOCK);
        if (err == -1) {
                fprintf (stderr, "Unable to set socket to nonblocking\n");
                close (socket_fd);
                exit (1);
        }
}

/* Function that sets stdin to nonblocking so that attempts to read from it
 * can fail without perminantly waiting on input. Also sets up a user's
 * display message. */
void setup_input () {
	/* stdin has file descriptor 0 by definitiion. This is the same as
         * setting the socket above to nonblocking. Sockets can be treated
         * the same as files and as stdin, stdout, and stderr. */
        int flags = fcntl (0, F_GETFL, 0);
        if (flags == -1) {
                fprintf (stderr, "Unable to get stdin's flags\n");
                exit (1);
        }
        int err = fcntl (0, F_SETFL, flags | O_NONBLOCK);
        if (err == -1) {
                fprintf (stderr, "Unable to set stdin to nonblocking\n");
                exit (1);
        }
	input_offset = 0;
	display_prefix ();
}

/* Function that handles the life of the client. Client will switch between
 * checking from input from the user and checking for a response from the
 * server. */
void maintain_connection () {
	fd_set read_fds;
	fd_set error_fds;
	while (1) {
		FD_ZERO (&read_fds);
		FD_SET (0, &read_fds);
		FD_SET (socket_fd, &read_fds);
		FD_ZERO (&error_fds);
		FD_SET (socket_fd, &error_fds);
		select (socket_fd + 1, &read_fds, NULL, &error_fds, NULL);
		if (FD_ISSET (socket_fd, &error_fds)) {
			handle_server_disconnect ();
		}
		if (FD_ISSET (0, &read_fds)) {
			process_input ();
		}
		if (FD_ISSET (socket_fd, &read_fds)) {
			receive_messages ();
		}
	}
}

/* Function used to handle messages being received from the server. It will
 * read in as much information as is available from the server and will then
 * process the message when a full message is successfully read. A message is
 * complete once a \n is found. */
void receive_messages () {
	/* Uses the errno error handling process for c. C doesn't have an actual
	 * error type so one of the ways errors are returned is using errno. In
	 * this system the extern (meaning declared in a different file, in this
	 * case errno.h gives the extern definition) integer errno will be
	 * assigned a value upon completion of the function, which will hold a
	 * different value depending on the reason for failure. This are all
	 * stored as macros which can be checked directly for the specific error
	 * reason. */
	int send_len;
	errno = 0;
	int length = read (socket_fd, server_message + server_offset, MAX_MESSAGE_LENGTH - server_offset);
	if (length > 0) {
		server_message[length + server_offset] = 0;
		/* Message could contain more than one message or not a complete message
		 * so each message end should be checked. */
		while ((send_len = find_message_end (server_message, server_offset)) != -1) {
			char *msg = generate_message (server_message, send_len + 1);
			handle_server_message (msg);
			free (msg);
			server_offset = 0;
		}
		server_offset = strlen (server_message);
	/* If the read fails because of a nonblocking reason the error should be ignored because
	 * this is the intended behavior. */
	} else if (length == 0 || (errno && errno != EAGAIN && errno != EWOULDBLOCK)) {
		handle_server_disconnect ();
	}
}
