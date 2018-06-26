#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>


pid_t launch_client (char *name, char *port, char *input_name, char *output_name);

unsigned wait_for_server (int server_fd, unsigned start, unsigned end);

void parse_prefix (int fd);

void read_output_line (int fd);

void cleanup_program ();

void wait_for_sync ();

void allocation_failed ();

void handle_signals (int signum);

char *path;

pid_t client_pid;

int main (int argc, char *argv[]) {
	if (argc != 7) {
		fprintf (stderr, "Error. Program takes exactly 7 arguments\n");
		exit (1);
	}
	signal (SIGUSR1, handle_signals);
	signal (SIGINT, handle_signals);
	signal (SIGQUIT, SIG_DFL);
	signal (SIGTERM, SIG_DFL);
	signal (SIGTTOU, SIG_DFL);
	signal (SIGTSTP, SIG_DFL);


	path = argv[0];
	int i;
	for (i = strlen (path); i >= 0; i--) {
		if (path[i] == '/') {
			path[i - 1] = 0;
			break;
		}
	}
	if (i == -1) {
		path = "";
	}
	char *name = argv[1];
	char *port = argv[2];
	char *input_file = argv[3];
	char *output_file = argv[4];
	char *intermediate_file = argv[5];
	char *server_file = argv[6];
	FILE *input_feeder = fopen (intermediate_file, "w+");
	if (input_feeder == NULL) {
		fprintf (stderr, "Error. Unable to create input stream to client\n");
		exit (1);
	}
	FILE *actual_input = fopen (input_file, "r");
	if (actual_input == NULL) {
		fprintf (stderr, "Error. Unable to open input file\n");
		exit (1);
	}
	int server_fd = open (server_file, O_RDWR | O_CREAT, 0777);
	if (server_fd < 0) {
		fprintf (stderr, "%s\n", server_file);
		fprintf (stderr, "Error. Unable to open server output\n");
		exit (1);
	}

	int client_fd = open (output_file, O_RDWR | O_CREAT, 0777);
	if (client_fd < 0) {
		fprintf (stderr, "Error. Unable to open client output\n");
		exit (1);
	}

	/* Launch the client. */
	client_pid = launch_client (name, port, intermediate_file, output_file);

	/* Enter a read loop to determine when to feed input. */
	char **buffer = malloc (sizeof (char *));
	if (buffer == NULL) {
		allocation_failed ();
	}
	size_t n = 0;
	int waiting = 0;
	int server_line = 0;
	int server_dest = 0;
	parse_prefix (client_fd);
	while (getline (buffer, &n, actual_input) != -1) {
		char *msg = buffer[0];
		bool send_or_receive = false;
		switch (msg[0]) {
			case '-':
				fwrite (msg + 2, 1, strlen (msg + 2), input_feeder);
				fflush (input_feeder);
				wait_for_sync ();
				send_or_receive = true;
				break;
			case 'S':
				msg[strlen (msg) - 1] = 0;
				server_dest = atoi (msg + 2);
				server_line = wait_for_server (server_fd, server_line, server_dest);
				break;
			default:
				msg[strlen (msg) - 1] = 0;
				waiting = atoi (msg);
				send_or_receive = true;
		}
		free (msg);
		buffer[0] = NULL;
		while (waiting) {
			read_output_line (client_fd);
			waiting--;
		}
		if (send_or_receive) {
			parse_prefix (client_fd);
		}
	}
	fwrite (" ", 1, 1, input_feeder);
	close (server_fd);
	close (client_fd);
	fclose (input_feeder);
	while (1) {
		wait_for_sync ();
	}
}

void parse_prefix (int fd) {
	struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 10000;
        int x;
        fd_set readfds;
        FD_ZERO (&readfds);
        FD_SET (fd, &readfds);
	for (int i = 0; i < 5; i++) {
        	x = select (fd + 1, &readfds, NULL, NULL, &timeout);
		if (x <= 0) {
			fprintf (stderr, "Error reading from output file\n");
			cleanup_program ();
			exit (1);
		}
                char c = 0;
                while (read (fd, &c, 1) == 0) {
			wait_for_sync ();
		}
                timeout.tv_sec = 0;
                timeout.tv_usec = 10000;
                FD_ZERO (&readfds);
                FD_SET (fd, &readfds);
        }
}

pid_t launch_client (char *name, char *port, char *input_name, char *output_name) {
	char *args[5];
	char *client_name = "../client";
	args[0] = malloc (sizeof (char) * (strlen (client_name) + strlen (path)));
	sprintf (args[0], "%s%s", path, client_name);
	args[1] = name;
	args[2] = "127.0.0.1";
	args[3] = port;
	args[4] = NULL;
	pid_t child_name = fork ();
	if (child_name == 0) {
		freopen (input_name, "r+", stdin);
		freopen (output_name, "w+", stdout);
		execv (args[0], args);
		fprintf (stderr, "Error. Unable to launch client\n");
		exit (1);
	} else if (child_name > 0) {
		return child_name;
	} else {
		fprintf (stderr, "Error unable to start client.");
		exit (1);
	}
}

unsigned wait_for_server (int server_fd, unsigned start, unsigned end) {
	int i;
	for (i = 0; i < end - start; i++) {
		read_output_line (server_fd);
	}
	return end;
}

void read_output_line (int fd) {
	/* Set a timeout value of half a second. */
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 10000;
	size_t readlen;
	int x;
	fd_set readfds;
	FD_ZERO (&readfds);
	FD_SET (fd, &readfds);
	while ((x = select (fd + 1, &readfds, NULL, NULL, &timeout)) > 0) {
		char c = 0;
		readlen = read (fd, &c, 1);
		if (c == '\n') {
			break;
		}
		if (readlen == 0) {
			wait_for_sync ();
		}
		timeout.tv_sec = 0;
		timeout.tv_usec = 10000;
		FD_ZERO (&readfds);
		FD_SET (fd, &readfds);
	}
	if (x <= 0) {
		fprintf (stderr, "Error reading from output file\n");
		cleanup_program ();
		exit (1);
	}

}

void wait_for_sync () {
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 10000;
	select (0, NULL, NULL, NULL, &timeout);
	if (waitpid (client_pid, NULL, WNOHANG) > 0) {
		cleanup_program ();
		exit (0);
	}
}

void cleanup_program () {
	kill (client_pid, SIGKILL);
}

void allocation_failed () {
	fprintf (stderr, "Ran out of memory\n");
	cleanup_program ();
	exit (1);
}

void handle_signals (int signum) {
	kill (client_pid, SIGKILL);
	waitpid (client_pid, NULL, 0);
	exit (0);
}
