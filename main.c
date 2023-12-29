#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>

#include "connmgr.h"
#include "sbuffer.h"
#include "sensor_db.h"
#include "datamgr.h"

// Global variables
pthread_mutex_t buffer_lock;     // Mutex for synchronizing access to buffer
pthread_cond_t empty_buffer;      // Buffer condition
sensor_buffer_t *buffer;          // Shared sensor buffer
char pipe_buffer[500000000];      // Shared pipe between child and parent
int p[2];                          // Pipe
int seqNum = 1;                    // Sequence number in log file
int port;                          // Port number parsed from command line arguments given to connection manager

// Function prototypes
void *connection_handler_func(void *arg);
void *data_processor_func(void *arg);
void *storage_manager_func(void *arg);

int main(int argc, char *argv[]) {

    // Parse port number from command line arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    port = atoi(argv[1]);

    // Initialize buffer & mutex
    int init_result = sensor_buffer_init(&buffer, buffer_lock);
    if (init_result != SBUFFER_SUCCESS) {
        printf("Buffer initialization failed\n");
        return 1;
    }

    pthread_t connection_handler;
    pthread_t data_processor;
    pthread_t storage_manager;

    // Check pipe for child & parent process
    if (pipe(p) < 0) {
        exit(1);
        printf("Failed to create pipe");
    }

    // Create child process
    pid_t pid = fork();

    if (pid < 0) {
        exit(2);
        printf("Failed to fork");
    }

    // WE ARE IN CHILD PROCESS
    if (pid == 0) {
        FILE *gateway_log = fopen("gateway_log", "w");  // Open log file for writing
        if (gateway_log == NULL) {
            perror("Error opening log file");
            exit(1);
        }

        // Read from pipe
        while ((read(p[0], pipe_buffer, sizeof(pipe_buffer))) > 0) {
            close(p[1]); // Close writing end of pipe

            // Get only one log event at a time (delimiter is a new line)
            char *event = strtok(pipe_buffer, "\n");

            while (event != NULL) {
                // Get current time as a string
                time_t mytime = time(NULL);
                char *time_str = ctime(&mytime);
                time_str[strlen(time_str) - 1] = '\0';

                // Write into log file
                fprintf(gateway_log, "<%d> <%s> <%s>\n", seqNum, time_str, event);
                seqNum++;
                event = strtok(NULL, "\n");  // event = next string
            }
            close(p[0]); // Close reading pipe when done
        }

        fclose(gateway_log); // Close log file when done
        exit(3);
    }

    // WE ARE IN PARENT PROCESS
    else {
        // Create threads
        pthread_create(&connection_handler, NULL, connection_handler_func, NULL);
        pthread_create(&data_processor, NULL, data_processor_func, NULL);
        pthread_create(&storage_manager, NULL, storage_manager_func, NULL);

        // Wait for threads to finish
        pthread_join(connection_handler, NULL);
        pthread_join(data_processor, NULL);
        pthread_join(storage_manager, NULL);

        pthread_mutex_destroy(&buffer_lock);
        pthread_cond_destroy(&empty_buffer);
        sensor_buffer_free(&buffer);
        pthread_exit(NULL);
    }

    return 0;
}

void *connection_handler_func(void *arg) {
    // Start server & write to buffer
    connection_handler_main(port, buffer, buffer_lock, empty_buffer);
    pthread_exit(NULL);
    return NULL;
}

void *data_processor_func(void *arg) {
    FILE *sensor_map = fopen("room_sensor.map", "r");
    data_processor_main(sensor_map, buffer, p, buffer_lock, empty_buffer);
    pthread_exit(NULL);
    return NULL;
}

void *storage_manager_func(void *arg) {
    pthread_mutex_lock(&buffer_lock);

    // Make and open CSV file for write
    FILE *csv = open_sensor_database("csv", false);
    sensor_buffer_node_t *node;
    sensor_data_t data;

    while (1) {
        if (buffer == NULL) {
            pthread_mutex_unlock(&buffer_lock);
            return NULL;
        }
        node = buffer->head;
        if (node == NULL) {  // Buffer is empty
            while (node == NULL) {
                pthread_cond_wait(&empty_buffer, &buffer_lock); // Wait for data
                printf("\nWaiting for data\n");
            }
        }
        pthread_cond_signal(&empty_buffer);

        while (node != NULL) {
            data = node->data;

            if (node->read_by_storagemgr == 1) {
                if (node->read_by_datamgr == 1) {
                    sensor_buffer_remove(buffer, &data, buffer_lock); // Remove node if read by all threads
                }
            } else {
                insert_sensor_data(csv, data.id, data.value, data.ts);
                node->read_by_storagemgr = 1;
            }

            node = node->next;
        }
    }

    pthread_mutex_unlock(&buffer_lock);
    close_sensor_database(csv); // Close CSV file
    pthread_exit(NULL);
    return NULL;
}

void write_into_log_pipe(char *msg) {
    write(p[1], msg, strlen(msg));
}
