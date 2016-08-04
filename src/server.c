#include "server.h"

static int respond(int socket);
static void get_info(char* port, struct addrinfo** servinfo);
static void sig_handler(int s);
static int setup_socket(char* port, struct addrinfo* servinfo);
static int respond(int socket);
static int send_all(int socket, char* msg);
static int get_req(int socket, char* path);
static int send_all_file(int socket, char* filename);

char* ROOT;

int serve_http(char* port) {
    // Info about the client's address
    struct sockaddr_storage clientaddr;
    socklen_t addr_size;
    struct sigaction sa;

    struct addrinfo *servinfo = NULL; // Gonna fill this out with getaddrinfo
    int sockdesc, newsock;

    ROOT = getenv("PWD");
    
    sockdesc = setup_socket(port, servinfo);

    freeaddrinfo(servinfo); // Free the addrinfo struct
    
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("listen");
        return 1;
    }

    printf("Server listening on port %s...\n", port);
                            
    for (;;) {
        listen(sockdesc, BACKLOG);

        addr_size = sizeof(clientaddr);
        newsock = accept(sockdesc, ((struct sockaddr *) &clientaddr), &addr_size);
        if (newsock == -1) {
            perror("accept");
            return 1;
        }

        if (!fork()) {
            close(sockdesc);
            respond(newsock);
            close(newsock);
            exit(0);
        }
        close(newsock);
    }

    close(sockdesc);

    return 0;
}

/*
 * Recieves HTTP request from a client connected over socket and sends
 * back requested resource.
 */
static int respond(int socket) {
    char rec_msg[99999];
    char* request[3];
    char* reqline;
    int bytes_rec, bytes_sent;
    
    memset(rec_msg, 0, sizeof(rec_msg));

    bytes_rec = recv(socket, rec_msg, sizeof(rec_msg), 0);
    printf("Msg: %d\n%s\n", bytes_rec, rec_msg);

    reqline = strtok(rec_msg, "\n");
    request[0] = strtok(reqline, " \t");
    request[1] = strtok(NULL, " \t");
    request[2] = strtok(NULL, " \t");
    char* http10 = "HTTP/1.0";
    char* http11 = "HTTP/1.1";

    if (strncmp(request[2], http10, strlen(http10)) != 0
            && strncmp(request[2], http11, strlen(http11)) != 0) {
        bytes_sent = send_all(socket, "HTTP/1.0 400 Bad Request\n");
    } else {
        printf("Request type: %s\n", request[0]);
        if (strncmp(request[0], "GET", 3) == 0) {
            get_req(socket, request[1]);
        } else {
            bytes_sent = send_all(socket, "Only supports GET\n");
        }
    }

    return bytes_sent;
}

/*
 * Responds to get requests, sending back requested file resources
 */
static int get_req(int socket, char* path) {
    int len = strlen(ROOT) + strlen(path);
    char fullpath[len];
    strncpy(fullpath, ROOT, strlen(ROOT) + 1);
    printf("Root: %s\n", ROOT);
    if (strncmp(path, "/", strlen(path)) == 0) {
        strncat(fullpath, "/index.html", strlen("/index.html"));
    } else {
        strncat(fullpath, path, strlen(path));
    }
    send_all_file(socket, fullpath);

    return 1;
}

/*
 * Sends packets to client connected on socket until all of msg has
 * been sent.
 */
static int send_all(int socket, char* msg) {
    return send(socket, msg, strlen(msg), 0);
}

/*
 * Same as send_all but sent data is read from a file specified by filename.
 */
static int send_all_file(int socket, char* filename) {
    int file = open(filename, O_RDONLY);
    int bytes_sent;
    int total_sent = 0;
    off_t offset = 0;
    struct stat stbuf;
    printf("File Desc: %d, File name: %s\n", file, filename);

    if (file == -1) {
        total_sent = send_all(socket, "HTTP/1.0 404 File not found\n");
    } else {
        fstat(file, &stbuf);
        while ((int) stbuf.st_size > total_sent) {
            bytes_sent = sendfile(socket, file, &offset, stbuf.st_size);
            if (bytes_sent == -1) {
                /*total_sent = send_all(socket, "HTTP/1.0 404 File not found\n");*/
                break;
            }
            total_sent += bytes_sent;
        }
        printf("Buff size: %d, Total sent: %d\n", (int) stbuf.st_size, total_sent);
        close(file);
    }


    return total_sent;
}

static void sig_handler(int s) {
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) < 0);
    errno = saved_errno;
}

/*
 * Opens a socket to listen for connections
 */
static int setup_socket(char* port, struct addrinfo* servinfo) {
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
static void get_info(char* port, struct addrinfo **servinfo) {
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
