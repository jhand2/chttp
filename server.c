#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <signal.h>

#define BACKLOG 10

int setup_socket(char* port, struct addrinfo* servinfo);
void get_info(char* port, struct addrinfo** servinfo);
void sig_handler(int s);

int main(int argc, char** argv) {
    // Info about the client's address
    struct sockaddr_storage clientaddr;
    socklen_t addr_size;
    struct sigaction sa;

    struct addrinfo *servinfo = NULL; // Gonna fill this out with getaddrinfo
    int sockdesc, newsock;

    if (argc != 2) {
        fprintf(stderr, "usage: server portnumber\n");
        return 1;
    }
    char* port = argv[1];
    
    sockdesc = setup_socket(port, servinfo);

    freeaddrinfo(servinfo); // Free that good addrinfo struct
    
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("listen");
        return 1;
    }

    printf("Server listening on port %s\n", port);
                            
    for (;;) {
        listen(sockdesc, BACKLOG);

        addr_size = sizeof(clientaddr);
        newsock = accept(sockdesc, ((struct sockaddr *) &clientaddr), &addr_size);
        if (newsock == -1) {
            perror("accept");
            return 1;
        }

        char* msg = "IT WORKS!\n";
        int len, bytes_sent;
        len = strlen(msg);

        if (!fork()) {
            close(sockdesc);
            bytes_sent = send(newsock, msg, len, 0);
            if (bytes_sent < len) {
                fputs("Not all the bytes were sent\n", stderr);
            }
            close(newsock);
            exit(0);
        }
        close(newsock);
    }

    close(sockdesc);

    return 0;
}

void sig_handler(int s) {
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) < 0);
    errno = saved_errno;
}

int setup_socket(char* port, struct addrinfo* servinfo) {
    struct addrinfo *info;
    int sockdesc = -1;
    get_info(port, &servinfo);

    for (info = servinfo; servinfo != NULL; info = info->ai_next) {
        // Ok cool, we made a socket
        sockdesc = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
        if (sockdesc == -1) {
            perror("socket");
            exit(1);
        }

        int yes = 1;
        int clear =
            setsockopt(sockdesc, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        if (clear == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (sockdesc != -1) {
            // Bind the above socket to the port we gave above.
            int bindstatus = bind(sockdesc, servinfo->ai_addr, servinfo->ai_addrlen);
            if (bindstatus == -1) {
                perror("bind");
                exit(1);
            }
            break;
        }
    }

    return sockdesc;
}

/*
 * Fills out the servinfo struct.
 */
void get_info(char* port, struct addrinfo **servinfo) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;        // Don't specify IP version
    hints.ai_socktype = SOCK_STREAM;    // TCP Stream Socket

    // Fill in IP for me. I think this says use localhost
    hints.ai_flags = AI_PASSIVE;        

    int status = getaddrinfo(NULL, port, &hints, servinfo);
    if (status != 0) {
        /*fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));*/
        exit(1);
    }
}
