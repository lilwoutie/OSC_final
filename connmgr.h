#ifndef _SERVER_MANAGER_H_
#define _SERVER_MANAGER_H_

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include "config.h"
#include "lib/tcpsock.h"
#include "sbuffer.h"

//#define PORT 5678
#define MAX_CONNECTIONS 3  // Specify the maximum number of connections the server will handle before exiting

int main_connection_manager(int port, sbuffer_t *buffer, pthread_mutex_t lock, pthread_cond_t condition);
void *handle_connection(void *client);

#endif /* _SERVER_MANAGER_H_ */
