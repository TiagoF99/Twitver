#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include "socket.h"

#ifndef PORT
    #define PORT 10000
#endif

#define LISTEN_SIZE 5
#define INVALID_USER "Invalid Username. Try Again.\r\n"
#define USER_TAKEN "Username already taken. Try Again.\r\n"
#define INVALID_INPUT "Invalid command.\r\n"
#define MSGLIM_REACHED "You have reached your message limit. You cant send anymore.\r\n"
#define FOLLOWLIM_REACHED "You or the person you are trying to follow have reached your following limits. Follow unsuccesful.\r\n"
#define QUIT_MESSAGE "quit"
#define WELCOME_MSG "Welcome to CSC209 Twitter! Enter your username: "
#define SEND_MSG "send"
#define SHOW_MSG "show"
#define FOLLOW_MSG "follow"
#define UNFOLLOW_MSG "unfollow"
#define BUF_SIZE 256
#define MSG_LIMIT 8
#define FOLLOW_LIMIT 5

struct client {
    int fd;
    struct in_addr ipaddr;
    char username[BUF_SIZE];
    char message[MSG_LIMIT][BUF_SIZE];
    struct client *following[FOLLOW_LIMIT]; // Clients this user is following
    struct client *followers[FOLLOW_LIMIT]; // Clients who follow this user
    char inbuf[BUF_SIZE]; // Used to hold input from the client
    char *in_ptr; // A pointer into inbuf to help with partial reads
    struct client *next;
};


// Provided functions. 
void add_client(struct client **clients, int fd, struct in_addr addr, int initial);
void remove_client(struct client **clients, int fd);


// The set of socket descriptors for select to monitor.
// This is a global variable because we need to remove socket descriptors
// from allset when a write to a socket fails. 
fd_set allset;

/* 
 * Create a new client, initialize it, and add it to the head of the linked
 * list.
 */
void add_client(struct client **clients, int fd, struct in_addr addr, int initial) {
    struct client *p = malloc(sizeof(struct client));
    if (!p) {
        perror("malloc");
        exit(1);
    }

    if (initial) {
        printf("Adding client %s\n", inet_ntoa(addr));
    }
    p->fd = fd;
    p->ipaddr = addr;
    p->username[0] = '\0';
    p->in_ptr = p->inbuf;
    p->inbuf[0] = '\0';
    p->next = *clients;

    for (int j = 0; j < FOLLOW_LIMIT; j++) {
        p->following[j] = NULL;
        p->followers[j] = NULL;
    }

    // initialize messages to empty strings
    for (int i = 0; i < MSG_LIMIT; i++) {
        p->message[i][0] = '\0';
    }

    *clients = p;
}

/* 
 * Remove client from the linked list and close its socket.
 * Also, remove socket descriptor from allset.
 */
void remove_client(struct client **clients, int fd) {
    struct client **p;

    for (p = clients; *p && (*p)->fd != fd; p = &(*p)->next)
        ;

    // Now, p points to (1) top, or (2) a pointer to another client
    // This avoids a special case for removing the head of the list
    if (*p) {
        printf("Disconnect from %s\n", inet_ntoa((*p)->ipaddr));
        struct client *copy = *clients;

        while (copy) {
            // send Goodbye user message to all active clients
            if (strcmp(copy->username, (*p)->username) != 0) {
                char buf[BUF_SIZE];
                strcpy(buf, "Goodbye ");
                strcat(buf, (*p)->username);
                strcat(buf, "\r\n");
                write(copy->fd, buf, strlen(buf));
            }

            //remove from follower and following lists
            for (int n=0; n < FOLLOW_LIMIT; n++) {

                // remove from other peoples following lists and follower lists
                if (copy->following[n] != NULL) {
                    if (strcmp(copy->following[n]->username, (*p)->username) == 0) {
                        fprintf(stderr, "%s is no longer following %s because they disconnected\n", copy->username, (*p)->username);
                        free(copy->following[n]);
                        copy->following[n] = NULL;
                    }
                }
                if (copy->followers[n] != NULL) {
                    if (strcmp(copy->followers[n]->username, (*p)->username) == 0) {
                        fprintf(stderr, "%s no longer has %s as a follower because they disconnected\n", copy->username, (*p)->username);
                        free(copy->followers[n]);
                        copy->followers[n] = NULL;
                    }
                }

                // free all items in your following and follower lists
                if (strcmp(copy->username, (*p)->username) == 0) {
                    if (copy->followers[n] != NULL) {
                        free(copy->followers[n]);
                        copy->followers[n] = NULL;
                    }
                    if (copy->following[n] != NULL) {
                        free(copy->following[n]);
                        copy->following[n] = NULL;
                    }
                }
            }
            copy=copy->next;
        }

        // Remove the client
        struct client *t = (*p)->next;
        printf("Removing client %d %s\n", fd, inet_ntoa((*p)->ipaddr));
        FD_CLR((*p)->fd, &allset);
        close((*p)->fd);
        free(*p);
        *p = t;
    } else {
        fprintf(stderr, "Trying to remove fd %d, but I don't know about it\n", fd);
    }
}

int find_network_newline(const char *buf, int n) {
    int i = 0;
    while (i < n) {
        if (buf[i] == '\r') {
            if (i + 1 < n) {
                if (buf[i+1] == '\n') {
                    return i + 2;
                }
            }
        }
        i++;
    }
    return -1;
}

int getlen_following(struct client *copy) {
    int count = 0;
    for (int l=0; l < FOLLOW_LIMIT; l++) {
        if (copy->following[l] != NULL) {
            count++;
        }
    }
    return count;
}

int getlen_followers(struct client *copy) {
    int count = 0;
    for (int l=0; l < FOLLOW_LIMIT; l++) {
        if (copy->followers[l] != NULL) {
            count++;
        }
    }
    return count;
}

int check_in_list(struct client *copy, char *username) {
    for (int l=0; l < FOLLOW_LIMIT; l++) {
        if (copy->following[l] != NULL) {
            if (strcmp(copy->username, username) == 0) {
                return -1;
            }
        }
    }
    return 0;
}

/*
check if username passed in by user trying to connect doesnt equal a username
already present
*/
int check_user_ne(struct client *copy, char *buf, int cur_fd, struct client **new_clients) {
    while (copy) {
        if (strcmp(copy->username, buf) == 0) {
           if (write(cur_fd, USER_TAKEN, strlen(USER_TAKEN)) == -1) {
                remove_client(new_clients, cur_fd);
            }
            fprintf(stderr, "user trying to add username: %s", INVALID_USER);
            return 0;
        }
        copy=copy->next;
    }
    return 1;
}

/*
If cur_fd inputs show, execute following.
*/
void show(struct client **active_clients, struct client **p, int cur_fd) {
    for (int h=0; h < FOLLOW_LIMIT; h++) {
        struct client *copy_act = *active_clients;
        while (copy_act) {
            if ((*p)->following[h] != NULL) {
                if (strcmp((*p)->following[h]->username, copy_act->username) == 0) {
                    for (int x = 0; x < MSG_LIMIT; x++) {
                        if (strcmp(copy_act->message[x], "\0") != 0 ) {
                            char show[BUF_SIZE];
                            strcpy(show, copy_act->username);
                            strcat(show, " wrote: ");
                            strcat(show, copy_act->message[x]);
                            strcat(show, "\r\n");
                            if (write(cur_fd, show, strlen(show)) == -1) {
                                remove_client(active_clients, cur_fd);
                            }
                        }
                    }
                }
            }
            copy_act=copy_act->next;
        }
    }
}

/*
Execute for cur_fd to follow person with username buf
*/
void follow(char *buf, struct client **active_clients, struct client **q, int cur_fd) {
    char *ptr = strchr(buf, ' ');
    if (!ptr) {
         if (write(cur_fd, INVALID_INPUT, strlen(INVALID_INPUT)) == -1) {
            remove_client(active_clients, cur_fd);
        }
        fprintf(stderr, "%s", INVALID_INPUT);
        return;
    }

    char *username_to_follow = ptr + 1;
    
    struct client *copy = *active_clients;
    struct client *p = *q;

    // check that it equals a persons username
    int check = 0;
    while (copy) {
        if (strcmp(copy->username, username_to_follow) == 0 && strcmp(username_to_follow, p->username) != 0) {
            check = 1; break;
        }
        copy=copy->next;
    }

    // it equals someones username
    if (check) {
        int follow_len = getlen_following(p);
        int following_len = getlen_followers(copy);

        // check that both lists have space
        if (follow_len < FOLLOW_LIMIT && following_len < FOLLOW_LIMIT) {
            //check that user doesnt already follow the username_to_follow
            if (check_in_list(p, username_to_follow) != -1) {
                for (int n=0; n < FOLLOW_LIMIT; n++) {
                    if (p->following[n] == NULL) {
                        add_client(&(p->following[n]), copy->fd, copy->ipaddr, 0);
                        strcpy(p->following[n]->username, username_to_follow);
                        break;
                    }
                }
                fprintf(stderr, "%s is following %s\n", p->username, username_to_follow);
                for (int n=0; n < FOLLOW_LIMIT; n++) {
                    if (copy->followers[n] == NULL) {
                        add_client(&(copy->followers[n]), p->fd, p->ipaddr, 0);
                        strcpy(copy->followers[n]->username, p->username);
                        break;
                    }
                }
                fprintf(stderr, "%s has %s as a follower\n", username_to_follow, p->username);
            } else {
                if (write(cur_fd, INVALID_INPUT, strlen(INVALID_INPUT)) == -1) {
                    remove_client(active_clients, cur_fd);
                }
                fprintf(stderr, "%s", INVALID_INPUT);
            }
        } else {
            if (write(cur_fd, FOLLOWLIM_REACHED, strlen(FOLLOWLIM_REACHED)) == -1) {
                remove_client(active_clients, cur_fd);
            }
            fprintf(stderr, "%s", FOLLOWLIM_REACHED);
        }
    } else {
        // doesnt equal anyones username so we send an invalid input message.
        if (write(cur_fd, INVALID_USER, strlen(INVALID_USER)) == -1) {
            remove_client(active_clients, cur_fd);
        }
        fprintf(stderr, "%s", INVALID_INPUT);
    }
}

/*
Execute for cur_fd to unfollow person with username buf
*/
void unfollow(char *buf, int cur_fd, struct client **q, struct client **act) {
    struct client *p = *q;
    struct client *active_clients = *act;

    char *ptr = strchr(buf, ' ');
    if (!ptr) {
         if (write(cur_fd, INVALID_INPUT, strlen(INVALID_INPUT)) == -1) {
            remove_client(act, cur_fd);
        }
        fprintf(stderr, "%s", INVALID_INPUT);
        return;
    }
    char *username_to_unfollow = ptr + 1;

    // check that it equals a persons username that you follow
    int check=0;
    for (int u=0; u < FOLLOW_LIMIT; u++) {
        if (p->following[u] != NULL) {
            if (strcmp(p->following[u]->username, username_to_unfollow) == 0) {check = 1; break;}
        }
    }

    // it equals someones username in your following list
    if (check) {
        struct client *copy= active_clients;
        while (active_clients) {

            if (strcmp(active_clients->username, username_to_unfollow) == 0) {

                // remove curr user from curr active clients followers list
                for (int j=0; j < FOLLOW_LIMIT; j++) {
                    if (active_clients->followers[j] != NULL) {
                        if (strcmp(active_clients->followers[j]->username, p->username) == 0) {
                            free(active_clients->followers[j]);
                            active_clients->followers[j] = NULL;
                            fprintf(stderr, "%s no longer has %s as a follower\n", username_to_unfollow, p->username);
                        }
                    }
                }
            }
            // remove from your following list
            for (int j=0; j < FOLLOW_LIMIT; j++) {
                if (p->following[j] != NULL) {
                    if (strcmp(p->following[j]->username, username_to_unfollow) == 0) {
                        free(p->following[j]);
                        p->following[j] = NULL;
                        fprintf(stderr, "%s unfollows %s\n", p->username, username_to_unfollow);
                    }
                }
            }

            active_clients = active_clients->next;
        }
        active_clients=copy;

    } else { // doesnt equal anyones username so we send an invalid input message.
        if (write(cur_fd, INVALID_USER, strlen(INVALID_USER)) == -1) {
            remove_client(act, cur_fd);
        }
        fprintf(stderr, "%s", INVALID_INPUT);
    }
}


/*
cur_fd sends message buf to all of his followers and stores input.
*/
void send_mess(char *buf, int cur_fd, struct client **q, struct client **act) {
    struct client *p = *q;
    struct client *active_clients = *act;
    
    char *ptr = strchr(buf, ' ');
    if (!ptr || (strcmp(ptr+1, "\0") == 0)) {
         if (write(cur_fd, INVALID_INPUT, strlen(INVALID_INPUT)) == -1) {
            remove_client(act, cur_fd);
        }
        fprintf(stderr, "%s", INVALID_INPUT);
        return;
    }
    char *message = ptr + 1;

    // check if you can send a message
    int g, mess_sent = 0;
    for (g=0; g < MSG_LIMIT; g++) {
        if (strcmp(p->message[g], "\0") != 0) {
            mess_sent++;
        } else {break;}
    }

    // if you have space to send another message
    if (mess_sent < MSG_LIMIT) {

        // send message to everyone that follows you
        for (int h=0; h < FOLLOW_LIMIT; h++) {
            struct client *copy_act = active_clients;
            while (copy_act) {
                if (p->followers[h] != NULL) {
                    if (strcmp(p->followers[h]->username, copy_act->username) == 0) {
                        char send[BUF_SIZE];
                        strcpy(send, p->username);
                        strcat(send, ": ");
                        strcat(send, message);
                        strcat(send, "\r\n");
                        if (write(copy_act->fd, send, strlen(send)) == -1) {
                            remove_client(act, copy_act->fd);
                        }
                    }
                }
                copy_act=copy_act->next;
            }   
        }

        // set message in your message array
        strcpy(p->message[g], message);
        if (strcmp(p->inbuf, "\0") == 0) {
            strcpy(p->inbuf, message);
        } else {
            p->inbuf[0] = '\0';
            strcpy(p->inbuf, message);
        }
    } else {
        if (write(cur_fd, MSGLIM_REACHED, strlen(MSGLIM_REACHED)) == -1) {
            remove_client(act, cur_fd);
        }
        fprintf(stderr, "%s\n", MSGLIM_REACHED);
    }
}


/*
read in input from client cur_fd
*/
void read_input(int cur_fd, int *inbuf, char *buf) {
    int room = sizeof(buf);  // How many bytes remaining in buffer?
    char *after = buf;       // Pointer to position after the data in buf
    int nbytes;
    while ((nbytes = read(cur_fd, after, room)) > 0) {
        fprintf(stderr, "[%d] Read %d bytes\n", cur_fd, nbytes);    
        *inbuf += nbytes;

        int where;
        if ((where = find_network_newline(buf, *inbuf)) > 0) {
            buf[where - 2] = '\0';
            break;
        }
        room = BUF_SIZE - (*inbuf);
        after = buf + (*inbuf);
    }
}


int main (int argc, char **argv) {
    int clientfd, maxfd, nready;
    struct client *p;
    struct sockaddr_in q;
    fd_set rset;

    // If the server writes to a socket that has been closed, the SIGPIPE
    // signal is sent and the process is terminated. To prevent the server
    // from terminating, ignore the SIGPIPE signal. 
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGPIPE, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    // A list of active clients (who have already entered their names). 
    struct client *active_clients = NULL;

    // A list of clients who have not yet entered their names. This list is
    // kept separate from the list of active clients, because until a client
    // has entered their name, they should not issue commands or 
    // or receive announcements. 
    struct client *new_clients = NULL;
    struct sockaddr_in *server = init_server_addr(PORT);
    int listenfd = set_up_server_socket(server, LISTEN_SIZE);
    free(server);
    // Initialize allset and add listenfd to the set of file descriptors
    // passed into select 
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

    // maxfd identifies how far into the set to search
    maxfd = listenfd;
    while (1) {
        
        // make a copy of the set before we pass it into select
        rset = allset;
        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if (nready == -1) {
            perror("select");
            exit(1);
        } else if (nready == 0) {
            continue;
        }

        // check if a new client is connecting
        if (FD_ISSET(listenfd, &rset)) {
            printf("A new client is connecting\n");
            clientfd = accept_connection(listenfd, &q);

            FD_SET(clientfd, &allset);
            if (clientfd > maxfd) {
                maxfd = clientfd;
            }
            printf("Connection from %s\n", inet_ntoa(q.sin_addr));
            add_client(&new_clients, clientfd, q.sin_addr, 1);
            char *greeting = WELCOME_MSG;
            if (write(clientfd, greeting, strlen(greeting)) == -1) {
                remove_client(&new_clients, clientfd);
            }
        } 
        // Check which other socket descriptors have something ready to read.
        // The reason we iterate over the rset descriptors at the top level and
        // search through the two lists of clients each time is that it is
        // possible that a client will be removed in the middle of one of the
        // operations. This is also why we call break after handling the input.
        // If a client has been removed, the loop variables may no longer be 
        // valid.

        int cur_fd, handled;
        for (cur_fd = 0; cur_fd <= maxfd; cur_fd++) {
            if (FD_ISSET(cur_fd, &rset)) {
                handled = 0;

                // Check if any new clients are entering their names
                for (p = new_clients; p != NULL; p = p->next) {
                    if (cur_fd == p->fd) {
                        
                        // Receive messages
                        char buf[BUF_SIZE] = {'\0'};
                        int inbuf = 0; // How many bytes currently in buffer?
                        while (strncmp(buf, "username:", 9) != 0) {
                            read_input(cur_fd, &inbuf, buf);
                            fprintf(stderr, "%s\n", buf);
                            buf[0] = '\0';

                        }

                        int invalid = 1;

                        char *ptr = strchr(buf, ':');
                        char *new = ptr + 1;
                        // check if username is empty string
                        if (inbuf <= 0 || (strcmp(buf, "\0") == 0)) {

                            if (write(cur_fd, INVALID_USER, strlen(INVALID_USER)) == -1) {
                                remove_client(&new_clients, cur_fd);
                            }
                            fprintf(stderr, "user trying to add username: %s", INVALID_USER);

                        }  else {

                            fprintf(stderr, "[%d] Found Newline: %s\n", p->fd, new);
                            
                            // check that it doesnt equal other peoples usernames
                            struct client *copy = active_clients;
                            int check = check_user_ne(copy, new, cur_fd, &new_clients);

                            if (check) { // if its a valid username

                                //remove from new and add to active
                                strcpy(new_clients->username, new);
                                struct client *copy = new_clients->next;
                                new_clients->next=active_clients;
                                active_clients=new_clients;
                                new_clients=copy;
                                fprintf(stderr, "%s has just joined.\n", new);

                                // notify all active users of your addition.
                                char user_welcome[BUF_SIZE];
                                strcpy(user_welcome, new);
                                strcat(user_welcome, " has just joined.\r\n");

                                struct client *ac;
                                for (ac = active_clients; ac != NULL; ac = ac->next) {
                                    if (write(ac->fd, user_welcome, strlen(user_welcome)) == -1) {
                                        remove_client(&active_clients, ac->fd);
                                    }                           
                                }
                                invalid = 0;
                            } 
                        }

                        // not a valid username so we resend the welcome message
                        if (invalid) {
                            if (write(cur_fd, WELCOME_MSG, strlen(WELCOME_MSG)) == -1) {
                                remove_client(&new_clients, cur_fd);
                            }
                        }

                        handled = 1;
                        break;
                    }
                }

                if (!handled) {
                    // Check if this socket descriptor is an active client
                    for (p = active_clients; p != NULL; p = p->next) {
                        if (cur_fd == p->fd) {

                            // read input from user
                            char buf[BUF_SIZE] = {'\0'};
                            int inbuf = 0; // How many bytes currently in buffer?
                            read_input(cur_fd, &inbuf, buf);

                            if (inbuf > 0) {
                                fprintf(stderr, "[%d] Found Newline %s\n", p->fd, buf);
                                fprintf(stderr, "%s: %s\n", p->username, buf);
                            } else {
                                fprintf(stderr, "[%d] Read 0 bytes\n", p->fd);
                                remove_client(&active_clients, cur_fd);
                            }
                            
                            // execute instructions depending on user input
                            if (strcmp(buf, SHOW_MSG) == 0) {

                                show(&active_clients, &p, cur_fd);

                            } else if (strncmp(buf, FOLLOW_MSG, strlen(FOLLOW_MSG)) == 0) {

                                follow(buf, &active_clients, &p, cur_fd);

                            } else if (strncmp(buf, UNFOLLOW_MSG, strlen(UNFOLLOW_MSG)) == 0) {

                                unfollow(buf, cur_fd, &p, &active_clients);

                            } else if (strncmp(buf, SEND_MSG, strlen(SEND_MSG)) == 0) {

                                send_mess(buf, cur_fd, &p, &active_clients);

                            } else if (strcmp(buf, QUIT_MESSAGE) == 0) {

                                remove_client(&active_clients, cur_fd);

                            } else {
                                // not a valid input
                                if (write(cur_fd, INVALID_INPUT, strlen(INVALID_INPUT)) == -1) {
                                    remove_client(&active_clients, cur_fd);
                                }
                                if (inbuf > 0) {
                                    fprintf(stderr, "%s", INVALID_INPUT);
                                }
                            }
                           
                            break;
                        }
                    }
                }
            }
        }
    }
    return 0;
}
