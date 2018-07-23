/* File that consists of functions used to handle the information regarding
 * users that have connected to the server.
 * Author: Yuriy Bash */

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "user_utils.h"
#include "client_server_utils.h"
#include "server.h"


/*
 * Function: create_name
 *
 * takes in a name and creates a fresh pointer with
 * the name copied over.
 *
 * name: pointer to name
 *
 * returns: new pointer
 *
 */
char *create_name (char *name) {
    char* name_ptr = malloc((strlen(name) + 1) * sizeof(char));
    strcpy(name_ptr, name);
    return name_ptr;
}

/*
 * Function: create_name_info
 *
 * takes in a name and outputs a new name_info struct having that name
 *
 * name: pointer to name
 *
 * returns: pointer to name_info struct
 *
 */
struct name_info *create_name_info (char *name) {
        struct name_info *ni = (struct name_info *) malloc(sizeof(struct name_info));
        ni->name = create_name(name);
        ni->total_tracking = 0;
        return ni;
}


/*
 * Function: create_name_info
 *
 * takes in a name and outputs a new struct user_info having that name.
 *
 * name: pointer to name
 *
 * returns: pointer to user_info struct
 *
 */
struct user_info *create_user (char *name) {

	struct user_info *ui = (struct user_info *) malloc(sizeof(struct user_info));
	ui->name_info = create_name_info(name);
	ui->nickname = NULL;
    ui->muted_total = (unsigned *) malloc(sizeof(unsigned));
	*(ui->muted_total) = 0;
    ui->muted_capacity = MAX_CONNECTIONS;

    struct name_info **muted_users = (struct name_info**) malloc(MAX_CONNECTIONS * sizeof(struct name_info*));
    ui->muted = muted_users;

	return ui;
}

/*
 * Function: cleanup_user
 *
 * frees the memory associated with a user. importantly, all references to that
 * user (through other users' collection of muted users) must be cleared first.
 *
 * name: pointer to user_info struct, the user to be freed
 *
 * returns: void
 *
 */
void cleanup_user (struct user_info *user) {

    for(int i = 0; i < MAX_CONNECTIONS; i++){
        if (users[i] == NULL || user == users[i]){
            continue;
        }

        for(int j = 0; j < MAX_CONNECTIONS; j++){
            if ((*users[i]).muted[j] == user->name_info){
                (*users[i]).muted[j] = NULL;
            }

        }
    }
    cleanup_name_info(user->name_info);
    free(user);

}

/*
 * Function: cleanup_name_info
 *
 * frees the memory associated with a name_info.
 *
 * name: pointer to name_info struct, the name_info to be freed.
 *
 * returns: void
 *
 */
void cleanup_name_info (struct name_info *info) {
    free(info);
}

/*
 * Function: istaken_name
 *
 * checks if name is already taken
 *
 * name: pointer to name to check
 *
 * returns: bool
 *
 */
bool istaken_name (char *name) {
        unsigned i = 1;
        unsigned ctr = 1;
        while (i < socket_total) {
                if (sockets[ctr] != -1) {
                        i++;
                        if (strcmp (name, users[ctr]->name_info->name) == 0) {
                                return true;
                        }
                }
                ctr++;
        }
        return false;
}

/*
 * Function: has_nickname
 *
 * checks if a user has a nickname
 *
 * name: pointer to user to check
 *
 * returns: bool
 *
 */
bool has_nickname (struct user_info *user) {
	if(user->nickname == NULL){
	    return false;
	} else {
	    return (strlen(*user->nickname) > 0);
	}
}

/* Function that takes in a name and determines if a user
 * has it as a nickname. You may find the global variables users,
 * socket_total, and sockets helpful here (which are defined as extern
 * in server.h). There are socket_total - 1 clients connected and they
 * can occupy indices starting at one. A client exists at an index i
 * iff sockets[i] != -1. Once you have seen all the clients you should
 * not continue checking later indices. Finally simply because a client
 * is connected does not mean they necessarily have user information
 * yet. If a client is connected at index i but does not have user
 * information yet users[i] will be NULL. */

/*
 * Function: istaken_nickname
 *
 * checks if a nickname is taken by any of the existing users.
 *
 * name: pointer to name to check
 *
 * returns: bool
 *
 */
bool istaken_nickname (char *name) {
    for(int i = 0; i < MAX_CONNECTIONS; i++){
        if (strcmp(*users[i]->nickname, name) == 0){
            return true;
        }
    }
    return false;
}

/*
 * Function: find_user
 *
 * finds a user based on the name. if a user with the name exists, a pointer
 * to the user_info is returned. otherwise, NULL is returned.
 *
 * name: pointer to name to check
 *
 * returns: pointer to user_info || NULL
 *
 */
struct user_info *find_user (char *name) {
        int start = 1;
        int ctr = 1;
        while (start < socket_total) {
                if (sockets[ctr] != -1) {
                        start++;
                        if (strcmp (users[ctr]->name_info->name, name) == 0) {
                                return users[ctr];
                        }
                }
                ctr++;
        }
        return NULL;
}


/*
 * Function: ismuted
 *
 * checks whether first user has muted the second user
 *
 * receiving_user: the first user
 * possibly_muted_user: the second user (who may have been muted)
 *
 * returns: bool
 *
 */
bool ismuted (struct user_info *receiving_user, struct user_info *possibly_muted_user) {

    struct name_info** muted_users = receiving_user->muted;
    for(int mu_ctr = 0; mu_ctr < MAX_CONNECTIONS; mu_ctr++){

        muted_users += mu_ctr;
        if(*muted_users == possibly_muted_user->name_info){
            return true;
        }

    }

    return false;

}
