#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define MAX_WAIT_COUNT 2000

pid_t start_server (char *output_dir, char *ref_dir);

pid_t start_client (char *input_dir, char *output_dir, char *name, char *server_output);

void read_file_response (int fd);

char *create_message (char **contents, unsigned arg_count);

unsigned calculate_expected_lines (char *dir);

void wait_for_sync ();

void cleanup_programs ();

void sort_clients ();

void proper_cleanup (char *input_dir, char *output_dir, char *ref_dir);

void quicksort (unsigned start, unsigned end);

void swap (unsigned a, unsigned b);

int setup_reference (char *output_dir, char *ref_dir, char *client_name, unsigned remaining);

void read_current_lines (int fd, char *expected_output, unsigned remaining);

void allocation_failed ();

void handle_signals (int signum);

char *path;

char *port;

pid_t server_pid;

pid_t client_pids[10];

char *client_names[10];

unsigned client_count;

bool check_mem;

unsigned wait_count;

int main (int argc, char *argv[]) {
	if (argc != 5 && argc != 6) {
		fprintf (stderr, "Error. File takes either 4 or 5 arguments, the input" \
			" directory, the output directory and a memflag\n");
		exit (1);
	}
	client_count = 0;
	server_pid = 0;
	signal (SIGUSR1, handle_signals);
        signal (SIGINT, handle_signals);
        signal (SIGQUIT, SIG_DFL);
        signal (SIGTERM, SIG_DFL);
        signal (SIGTTOU, SIG_DFL);
        signal (SIGTSTP, SIG_DFL);
	wait_count = 0;
	check_mem = false;
	path = argv[0] + 2;
	int k;
	for (k = strlen (path); k >= 0; k--) {
		if (path[k] == '/') {
			path[k + 1] = 0;
			break;
		}
	}
	if (k == -1) {
		path = "";
	}
	char *input_dir = argv[1];
	char *output_dir = argv[2];
	char *ref_dir = argv[3];
	port = argv[4];
	if (argc == 6) {
		if (strcmp ("--m", argv[5]) == 0) {
			check_mem = true;
		} else {
			fprintf (stderr, "Error unknown flag for memory flag");
			exit (1);
		}	
	}

	/* First start up the server. */
	server_pid = start_server (output_dir, ref_dir);
	/* Open up the list of clients to start. */
	char *client_names_end = "/nameslist";
	char *client_namelist = malloc (sizeof (char) * (strlen (input_dir) + strlen (client_names_end) + 1));
	sprintf (client_namelist, "%s%s", input_dir, client_names_end);

	/* Open up the list of server output messages. */
	char *output_end = "/server";
	char *server_output = malloc (sizeof (char) * (strlen (output_dir) + strlen (output_end) + 1));
	sprintf (server_output, "%s%s", output_dir, output_end);

	FILE *namelist = fopen (client_namelist, "r");
	if (namelist == NULL) {
		fprintf (stderr, "Error. Unable to open namelist\n");
		cleanup_programs ();
		exit (1);
	}
	int server_fd = open (server_output, O_CREAT | O_RDWR, 0777);
	if (server_fd < 0) {
		fprintf (stderr, "Error. Unable to open server output\n");
		cleanup_programs ();
		exit (1);
	}
	read_file_response (server_fd);
	/* Enter a loop of starting clients. */
	char **nameline = malloc (sizeof (char *));
	if (nameline == NULL) {
		allocation_failed ();
	}
	nameline[0] = NULL;
	size_t n = 0;
	unsigned i = 0;
	while (getline (nameline, &n, namelist) != -1) {
		nameline[0][strlen (nameline[0]) - 1] = 0;
		client_pids[i] = start_client (input_dir, output_dir, nameline[0], server_output);
		client_names[i] = nameline[0];
		read_file_response (server_fd);
		i++;
		client_count++;
		nameline[0] = NULL;
	}
	free (nameline[0]);
	free (nameline);
	fclose (namelist);
	i++;

	/* Determine the total number of lines in the expected server output. */
	unsigned total_lines = calculate_expected_lines (ref_dir);

	while (i < total_lines) {
		read_file_response (server_fd);
		i++;
	}
	wait_for_sync ();
	close (server_fd);

	/* Kill any remaining clients in alphabetical order.
	 * Then kill the server. */
	proper_cleanup (input_dir, output_dir, ref_dir);
}

pid_t start_server (char *output_dir, char *ref_dir) {
	char *contents[5];
	contents[0] = output_dir;
	contents[1] = "/server";
	char *file = create_message (contents, 2);
	char *args[7];
	char *path_parts[2];
	if (check_mem) {
		char *log_parts[3];
		args[0] = "/usr/bin/valgrind";
		args[1] = "--leak-check=full";
		args[2] = "--show-leak-kinds=all";
		log_parts[0] = "--log-file=";
		log_parts[1] = ref_dir;
		log_parts[2] = "valgrind_log.txt";
		args[3] = create_message (log_parts, 3);
		path_parts[0] = path;
		path_parts[1] = "../server";
		args[4] = create_message (path_parts, 2);
		args[5] = port;
		args[6] = NULL;
	} else {
		path_parts[0] = path;
		path_parts[1] = "../server";
		args[0] = create_message (path_parts, 2);
		args[1] = port;
		args[2] = NULL;
	}
	pid_t child_process = fork ();
	if (child_process == 0) {
		freopen (file, "w+", stdout);
		free (file);
		execv (args[0], args);
		fprintf (stderr, "Error unable to start server\n");
		exit (1);
	} else if (child_process > 0) {
		setpgid (child_process, child_process);
		free (file);
		if (check_mem) {
			free (args[3]);
			free (args[4]);
		} else {
			free (args[0]);
		}
		return child_process;
	} else {
		fprintf (stderr, "Error unable to start server\n");
		exit (1);
	}
}

pid_t start_client (char *input_dir, char *output_dir, char *name, char *server_output) {
	char *args[8];
	char *path_parts[2];
	path_parts[0] = path;
        path_parts[1] = "./run_user";
        args[0] = create_message (path_parts, 2);
	args[1] = name;
	args[2] = port; 
	args[3] = malloc (sizeof (char) * (5 + strlen (input_dir) + strlen (name)));
	args[4] = malloc (sizeof (char) * (6 + strlen (output_dir) + strlen (name)));
	args[5] = malloc (sizeof (char) * (6 + strlen (input_dir) + strlen (name)));
	if (args[3] == NULL || args[4] == NULL || args[5] == NULL) {
		allocation_failed ();
	}
	args[6] = server_output;
	args[7] = NULL;
	sprintf (args[3], "%s/%s.in", input_dir, name);
	sprintf (args[4], "%s/%s.out", output_dir, name);
	sprintf (args[5], "%s/%s.int", input_dir, name);
	pid_t child_process = fork();
	if (child_process == 0) {
		execv (args[0], args);
		fprintf (stderr, "Error unable to start client\n");
		cleanup_programs ();
		exit (1);
	} else if (child_process > 0) {
		setpgid (child_process, child_process);
		free (args[0]);
		free (args[3]);
		free (args[4]);
		free (args[5]);
		return child_process;
	} else {
		fprintf (stderr, "Error unable to start client.\n");
		cleanup_programs ();
		exit (1);
	}
}

void read_file_response (int fd) {
	/* Set a timeout value of half a second. */
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 100000;
	fd_set readset;
	FD_ZERO (&readset);
	FD_SET (fd, &readset);
	int x;
	char c = 0;
	unsigned size;
	while ((x = select (fd + 1, &readset, NULL, NULL, &timeout)) > 0) {
		size = read (fd, &c, 1);
		if (c == '\n') {
			break;
		}
		if (size == 0) {
			wait_for_sync ();
		}
		FD_ZERO (&readset);
	        FD_SET (fd, &readset);
		timeout.tv_sec = 0;
		timeout.tv_usec = 100000;
	}
	if (x <= 0) {
		fprintf (stderr, "Error reading from server file\n");
		cleanup_programs ();
		exit (1);
	}
}	

char *create_message (char **contents, unsigned arg_count) {
	unsigned total_length = 1;
	for (int i = 0; i < arg_count; i++) {
		total_length += strlen (contents[i]);
	}
	char *msg = malloc (sizeof (char) * total_length);
	msg[0] = 0;
	for (int i = 0; i < arg_count; i++) {
		strcat (msg, contents[i]);
	}
	return msg;
}

unsigned calculate_expected_lines (char *dir) {
	char *contents[2];
	contents[0] = dir;
	contents[1] = "/server";
	char *filename = create_message (contents, 2);
	FILE *file = fopen (filename, "r");
	if (file == NULL) {
		fprintf (stderr, "Error. Unable to open expected server output");
		cleanup_programs ();
		exit (1);
	}
	char **buffer = malloc (sizeof (char *));
	unsigned i = 0;
	size_t n = 0;
	while (getline (buffer, &n, file) != -1) {
		free (buffer[0]);
		i++;
		buffer[0] = NULL;
	}
	free (buffer[0]);
	free (buffer);
	return i;
}

void wait_for_sync () {
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 1000;
	select (0, NULL, NULL, NULL, &timeout);
	if (waitpid (server_pid, NULL, WNOHANG) > 0) {
		cleanup_programs ();
		exit (1);
	}
	wait_count++;
	if (wait_count == MAX_WAIT_COUNT) {
		cleanup_programs ();
		exit (1);
	}
}

void cleanup_programs () {
	for (int i = 0; i < client_count; i++) {
		free (client_names[i]);
		killpg (client_pids[i], SIGUSR1);
		wait_for_sync ();
	}
	kill (server_pid, SIGKILL);
}

void proper_cleanup (char *input_dir, char *output_dir, char *ref_dir) {
	sort_clients (input_dir);
	if (check_mem) {
		waitpid (server_pid, NULL, 0);
		for (int i = 0; i < client_count; i++) {
			waitpid (client_pids[i], NULL, 0);
			free (client_names[i]);
		}
	} else if (client_count > 0) {
		int output_fds[client_count];
		for (int i = 0; i < client_count; i++) {
			output_fds[i] = setup_reference (output_dir, ref_dir, client_names[i], i);
		}
		for (int i = 0; i < client_count - 1; i++) {
			free (client_names[i]);
			kill (client_pids[i], SIGUSR1);
			waitpid (client_pids[i], NULL, 0);
			close (output_fds[i]);
			for (int j = i + 1; j < client_count; j++) {
				read_file_response (output_fds[j]);
			}
		}
		close (output_fds[client_count - 1]);
		free (client_names[client_count - 1]);
		kill (client_pids[client_count - 1], SIGUSR1);
		waitpid (client_pids[client_count - 1], NULL, 0);
		kill (server_pid, SIGKILL);
	} else {
		kill (server_pid, SIGKILL);
	}
}

int setup_reference (char *output_dir, char *ref_dir, char *client_name, unsigned remaining) {
	char *file_parts[3];
	file_parts[0] = output_dir;
	file_parts[1] = client_name;
	file_parts[2] = ".out";
	char *filename = create_message (file_parts, 3);
	if (filename == NULL) {
		allocation_failed ();
	}
	int fd = open (filename, O_RDWR);
	if (fd < 0) {
                fprintf (stderr, "Error. Unable to open file to synch exits\n");
                cleanup_programs ();
                exit (1);
        }
	free (filename);
	char *ref_parts[3];
	ref_parts[0] = ref_dir;
	ref_parts[1] = client_name;
	ref_parts[2] = ".ref";
	char *reffile = create_message (ref_parts, 3);
	read_current_lines (fd, reffile, remaining);
	return fd;
}

void sort_clients (char *input_dir) {
	quicksort (0, client_count);
	char *file_parts[2];
	file_parts[0] = input_dir;
	file_parts[1] = "/remaining_names";
	char *filename = create_message (file_parts, 2);
	FILE *leftovers = fopen (filename, "r");
	free (filename);
	if (leftovers == NULL) {
		fprintf (stderr, "Error unable to open file of remaining clients\n");
		cleanup_programs ();
		exit (1);
	}
	char **buffer = malloc (sizeof (char *));
	if (buffer == NULL) {
		allocation_failed ();
	}
	buffer[0] = NULL;
	size_t n = 0;
	unsigned ctr = 0;
	unsigned pos = 0;
	while (getline (buffer, &n, leftovers) != -1 && ctr < client_count) {
		char *msg = buffer[0];
		msg [strlen (msg) - 1] = 0;
		if (strcmp (msg, client_names[ctr]) == 0) {
			if (pos != ctr) {
				client_names[pos] = client_names[ctr];
				client_pids[pos] = client_pids[ctr];
			}
			pos++;
		} else {
			free (client_names[ctr]);
		}
		free (msg);
		buffer[0] = NULL;
		ctr++;
	}
	free (buffer[0]);
	free (buffer);
	client_count = pos;
	fclose (leftovers);
}

void quicksort (unsigned start, unsigned end) {
	if (end - start <= 1) {
		return;
	}
	unsigned pivot = start + rand () % (end - start - 1);
	swap (pivot, start);
	pivot = start;
	unsigned i = start + 1;
	while (i < end) {
		if (strcmp (client_names[pivot], client_names[i]) > 0) {
			swap (pivot, i);
			pivot++;
			swap (pivot, i);
			if (pivot == i) {
				i++;
			}
		} else {
			i++;
		}
	}
	quicksort (start, pivot);
	quicksort (pivot + 1, end);

}

void swap (unsigned a, unsigned b) {
	pid_t temp = client_pids[a];
	client_pids[a] = client_pids[b];
	client_pids[b] = temp;
	char *name_temp = client_names[a];
	client_names[a] = client_names[b];
	client_names[b] = name_temp;
}

void read_current_lines (int fd, char *output_file, unsigned remaining) {
	FILE *expected_output = fopen (output_file, "r");
	if (expected_output == NULL) {
		fprintf (stderr, "Unable to open client expected output\n");
		cleanup_programs ();
		exit (1);
	}
	free (output_file);
	char **buf = malloc (sizeof (char *));
	size_t n = 0;
	unsigned j = 0;
	while (getline (buf, &n, expected_output) != -1) {
		free (buf[0]);
		buf[0] = NULL;
		j++;
	}
	fclose (expected_output);
	free (buf[0]);
	free (buf);
	size_t len;
	unsigned num_lines = j - (remaining) - 1;
	for (int i = 0; i < num_lines; i++) {
		char c = 0;
		while (c != '\n') {
			len = read (fd, &c, 1);
			if (len == 0) {
				wait_for_sync ();
			}
		}
	}
}

void allocation_failed () {
	fprintf (stderr, "Ran out of memory\n");
	cleanup_programs ();
	exit (1);
}

void handle_signals (int signum) {
	if (server_pid != 0) {
		cleanup_programs ();
	}
	fprintf (stderr, "Exiting due to interrupt\n");
	exit (1);
}
