/*
 * socket demonstrations:
 * This is the server side of an "internet domain" socket connection, for
 * communicating over the network.
 *
 * In this case we are willing to wait for chatter from the client
 * _or_ for a new connection.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef PORT
    #define PORT 57076
#endif

# define SECONDS 10
# define MAX_BUF 300
# define MAX_CLIENTS 1024

struct client {
    int fd;
    int loggedin;
    struct in_addr ipaddr;
    struct client *next;
};

struct game {
    struct client *client1;
    struct client *client2;
    int filled;
    int turn;
};

static struct client *addclient(struct client *top, int fd, struct in_addr addr);
static struct client *removeclient(struct client *top, int fd);
static struct client *getclient(struct client *head, int fd);
static void broadcast(struct client *top, char *s, int size);
int handleclient(struct client *p, struct client *top);
static int getgame(struct game* games[], int fd);


int bindandlisten(void);



int accept_player(int listen_soc) {

    // you must complete this function

    struct sockaddr_in client_addr;
    unsigned int client_len = sizeof(struct sockaddr_in);

    int client_socket = accept(listen_soc, (struct sockaddr *)&client_addr, &client_len);
    if (client_socket == -1) {
        perror("accept");
        exit(1);
    }
    char msg1[MAX_BUF];

    sprintf(msg1, "What is your name?\r\n");

    write(client_socket, msg1, strlen(msg1));

    return client_socket;
}

void read_a_login(int clientfd) {

    char msg2[MAX_BUF];
    char name[MAX_BUF];
    int num_chars = read(clientfd, name, MAX_BUF);
    
    name[num_chars] = '\0';

    // it may take more than one read to get all of the data that was written

    // if \r\n is not found, continue reading from socket
    while(strstr(name, "\r\n") == NULL) {
        num_chars += read(clientfd, &name[num_chars], MAX_BUF-num_chars);
        name[num_chars]='\0';
    }
    name[num_chars-2] = '\0';


    sprintf(msg2, "Welcome, %s! Awaiting opponent...\r\n", name);

    write(clientfd, msg2, strlen(msg2));
}

int main(void) {
    int clientfd, maxfd, nready;
    struct client *p;
    struct client *head = NULL;
    socklen_t len;
    struct sockaddr_in q;
    struct timeval tv;
    fd_set allset;
    fd_set rset;


    int listenfd = bindandlisten();
    // initialize allset and add listenfd to the
    // set of file descriptors passed into select
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);
    // maxfd identifies how far into the set to search
    maxfd = listenfd;

    struct game* games[MAX_CLIENTS];

    // initialize games list
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        // Allocate memory for each game
        games[i] = malloc(sizeof(struct game));
        if (games[i] == NULL) {
            // Handle memory allocation failure
            perror("malloc");
            exit(1);
        }

        // Initialize the structure's fields
        games[i]->client1 = NULL; // Assuming NULL is a "default" value
        games[i]->client2 = NULL;
        games[i]->filled = 1;
        games[i]->turn = 0;
    }

    while (1) {
        // make a copy of the set before we pass it into select
        rset = allset;
        /* timeout in seconds (You may not need to use a timeout for
        * your assignment)*/
        tv.tv_sec = SECONDS;
        tv.tv_usec = 0;  /* and microseconds */

        nready = select(maxfd + 1, &rset, NULL, NULL, &tv);
        if (nready == 0) {
            printf("No response from clients in %d seconds\n", SECONDS);
            continue;
        }

        if (nready == -1) {
            perror("select");
            continue;
        }   

        for(int i = 0; i <= maxfd; i++) {

            if (FD_ISSET(i, &rset)){
                if (i == listenfd)
                {
                    // listenerfd is ready, meaning there is a client
                    clientfd = accept_player(listenfd);


                    FD_SET(clientfd, &allset);
                    if (clientfd > maxfd) {
                        maxfd = clientfd;
                    }
                    head = addclient(head, clientfd, q.sin_addr);


                    for (int i = 0; i < 1024; i++) {
                        if (games[i]->filled == 1) {
                            if (games[i]->client1 == NULL) {
                                games[i]->client1 = head;
                            } else if (games[i]->client2 == NULL) {
                                games[i]->client2 = head;
                                games[i]->filled = 0;

                                // the players are now ready to play!
                            }
                            
                            break;
                        }
                    }

                } else {
                    // must be from a client if not the listener

                    for (p = head; p != NULL; p = p->next) {
                        if (p->fd == i) {
                            if (p->loggedin == 1) {
                                // this part is for the name input
                                p->loggedin = 0;
                                read_a_login(p->fd);
                            }
                            
                            int curr_game = getgame(games, p->fd);

                            printf("%d", curr_game);

                            // for (int j = 0; j < MAX_CLIENTS; j++)
                            // {
                            //     printf("%p\n", (void *)games[j]->filled);

                            // }
                            
                            // if (games[curr_game]->filled == 0) {
                            //     printf("start/continue the game");
                            // }
                            break;
                        }
                    }
                }
            }
        }
    }


    // Remember to free the allocated memory when done with the gamez
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        free(games[i]);
    }

    return 0;
}

 /* bind and listen, abort on error
  * returns FD of listening socket
  */
int bindandlisten(void) {
    struct sockaddr_in r;
    int listenfd;

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }
    int yes = 1;
    if ((setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))) == -1) {
        perror("setsockopt");
    }
    memset(&r, '\0', sizeof(r));
    r.sin_family = AF_INET;
    r.sin_addr.s_addr = INADDR_ANY;
    r.sin_port = htons(PORT);

    if (bind(listenfd, (struct sockaddr *)&r, sizeof r)) {
        perror("bind");
        exit(1);
    }

    if (listen(listenfd, 5)) {
        perror("listen");
        exit(1);
    }
    return listenfd;
}

static struct client *addclient(struct client *top, int fd, struct in_addr addr) {
    struct client *p = malloc(sizeof(struct client));
    if (!p) {
        perror("malloc");
        exit(1);
    }

    printf("Adding client %s\n", inet_ntoa(addr));

    p->fd = fd;
    p->ipaddr = addr;
    p->next = top;
    p->loggedin = 1;
    top = p;
    return top;
}

static struct client *removeclient(struct client *top, int fd) {
    struct client **p;

    for (p = &top; *p && (*p)->fd != fd; p = &(*p)->next)
        ;
    // Now, p points to (1) top, or (2) a pointer to another client
    // This avoids a special case for removing the head of the list
    if (*p) {
        struct client *t = (*p)->next;
        printf("Removing client %d %s\n", fd, inet_ntoa((*p)->ipaddr));
        free(*p);
        *p = t;
    } else {
        fprintf(stderr, "Trying to remove fd %d, but I don't know about it\n",
                 fd);
    }
    return top;
}

/*
* given an fd and the games array, find the game struct which has fd in it
*/
static int getgame(struct game* games[], int fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (games[i]->client1 != NULL) {
            if (games[i]->client1->fd == fd) {
                printf("client1->fd: %d\n", games[i]->client1->fd);
                return i;
            }
        } else if (games[i]->client2 != NULL) {
            printf("lo");
            if (games[i]->client2->fd == fd) {
                return i;
            }
        }
        
    }

    // no game exists, which should be impossible
    perror("getgame error");
    exit(1);
}


static void broadcast(struct client *top, char *s, int size) {
    struct client *p;
    for (p = top; p; p = p->next) {
        write(p->fd, s, size);
    }
    /* should probably check write() return value and perhaps remove client */
}

void write_to_players(char *msg, int player1, int player2) {
    // you must complete this function
    write(player1, msg, strlen(msg));
    write(player2, msg, strlen(msg));
}