#include "connmgr.h"

int received_bytes, receive_result;
int connection_count = 0;
sensor_data_t received_data;

pthread_mutex_t buffer_lock;           // Mutex for synchronizing access to buffer
pthread_cond_t buffer_condition;       // Buffer condition
sbuffer_t *shared_buffer;              // Shared buffer

void *handle_connection(void *client) {
    char log_message[LOG_MESSAGE_SIZE];
    int first_measurement = 1;

    do {
        // Read sensor ID
        received_bytes = sizeof(received_data.id);
        receive_result = tcp_receive(client, (void *)&received_data.id, &received_bytes);
        // Read temperature
        received_bytes = sizeof(received_data.value);
        receive_result = tcp_receive(client, (void *)&received_data.value, &received_bytes);
        // Read timestamp
        received_bytes = sizeof(received_data.ts);
        receive_result = tcp_receive(client, (void *)&received_data.ts, &received_bytes);

        if (first_measurement == 1) {
            sprintf(log_message, "New connection from Sensor %d.\n", received_data.id);
            write_into_log_pipe(log_message);
            first_measurement = 0;
        }

        if ((receive_result == TCP_NO_ERROR) && received_bytes) {
            printf("Sensor ID: %" PRIu16 " - Temperature: %g - Timestamp: %ld\n",
                   received_data.id, received_data.value, (long int)received_data.ts);
            if (sbuffer_insert(shared_buffer, &received_data, buffer_lock, buffer_condition) == SBUFFER_SUCCESS)
                printf("Measurement added to the buffer\n");
        }
    } while (receive_resul
