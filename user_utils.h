/* File that consists of functions used to handle the information regarding
 * users that have connected to the server.
 * Author: Nick Riasanovsky */

#ifndef USER_UTILS_H
#define USER_UTILS_H

/* Struct containing the information about a user. */
struct user_info {
        struct name_info *name_info;
        char **nickname;
        unsigned *muted_total;
        unsigned muted_capacity;
        struct name_info **muted; /* Contains a dynamically sized collection of 
                                   * muted_total distinct ptrs to name_info structs, 
                                   * each of which is either the name_info field
                                   * of a current user or the name_info field
                                   * of a user that has disconnected (which
                                   * resulted in calling cleanup_user on that
                                   * user_info struct pointer). 
                                   * muted_capacity is the maximum number of pointers
                                   * to name_info structs that can be stored in muted.
                                   * This can be resized dynamically (though this is 
                                   * not required). */
};

/* Struct containing the relevant information about
 * a user's name. total_tracking tells how many users
 * have a pointer to this struct and therefore may
 * attempt to access it. */
struct name_info {
        char *name;
        unsigned total_tracking;
};


/* Function that takes in a name and outputs a new user_info having
 * that name. The struct should be capable or producing correct
 * functionality immediately. You cannot assume that name would remain
 * a valid pointer after the function exits. */
struct user_info *create_user (char *name);

/* Function that takes in a name and outputs a new name_info
 * struct having that name. You cannot assume the name is valid
 * after the function exits. */
struct name_info *create_name_info (char *name);

/* Function that takes in a name and creates a fresh pointer with
 * the name copied over. */
char *create_name (char *name);

/* Function that frees the memory associated with a user. Should not free
 * any pointer that will be accessed again. */
void cleanup_user (struct user_info *user);

/* Function that frees the memory assoicated with a name info. Should not
 * free any pointer that will need to be accessed again. */
void cleanup_name_info (struct name_info *info);

/* Function that takes in a name and determines if it is already a user's
 * name. */
bool istaken_name (char *name);

/* Function that determines if a user has a nickname. */
bool has_nickname (struct user_info *user);

/* Function that takes in a name and determines if a user
 * has it as a nickname. */
bool istaken_nickname (char *name);

/* Function that takes in a name and return the user who has that name as their
 * actual name. Returns NULL is no user has that actual name. */
struct user_info *find_user (char *name);

/* Takes in a two users and checks if the first user has muted the second
 * user. */
bool ismuted (struct user_info *receiving_user, struct user_info *possibly_muted_user);

#endif
