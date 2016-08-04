#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/wait.h>
#include <netdb.h>
#include <signal.h>

#define BACKLOG 10 // Max number of pending connections. Any new connections
                   // will be dropped until connections are processed.

/*
 * Starts an http server listening to requests on port. The current
 * implementation only serves static file requests.
 *
 * port: port on which http server listens to requests
 */
int serve_http(char* port);
