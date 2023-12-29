#include "datamgr.h"

typedef struct {
    uint16_t sensor_id;
    uint16_t room_id;
    double temp_avg;
    time_t last_modified;
    double temperatures[RUN_AVG_LENGTH];
    int nrOfTemp;
} my_element_t;

dplist_t *sensor_list;

void data_manager_main(FILE *fp_sensor_map, sbuffer_t *buffer, int p[2], pthread_mutex_t buffer_lock, pthread_cond_t condition) {
    my_element_t *current_sensor;
    sensor_list = dpl_create(element_copy, element_free, element_compare);
    current_sensor = malloc(sizeof(my_element_t));
    uint16_t sensor_id, room_id;

    char msg[LOG_MESSAGE_SIZE];

    // Read sensor map file & initiate all sensors
    while (feof(fp_sensor_map) == 0) {
        fscanf(fp_sensor_map, "%hd %hd\n", &room_id, &sensor_id);
        current_sensor->sensor_id = sensor_id;
        current_sensor->room_id = room_id;
        current_sensor->temp_avg = 0.0; // Set average to 0 for now
        current_sensor->last_modified = 0;
        current_sensor->nrOfTemp = 0;

        // Add sensor to linked list
        dpl_insert_at_index(sensor_list, current_sensor, dpl_size(sensor_list), true);

        if (feof(fp_sensor_map) != 0) {
            dpl_insert_at_index(sensor_list, current_sensor, dpl_size(sensor_list), true);
        }
    }

    // Read data from buffer
    pthread_mutex_lock(&buffer_lock);
    sbuffer_node_t *current_node = malloc(sizeof(sbuffer_node_t));
    current_node = buffer->head;

    if (current_node == NULL) {  // Buffer is empty
        while (current_node == NULL) {
            pthread_cond_wait(&condition, &buffer_lock); // Wait for data
            printf("\nWaiting for data\n");
        }
    }
    pthread_cond_signal(&condition);

    while (current_node != NULL) {
        sensor_data_t data = current_node->data;
        sensor_id_t id;
        sensor_value_t temperature;
        sensor_ts_t timestamp;

        if (current_node->read_by_datamgr == 1) {
            if (current_node->read_by_storagemgr == 1)
                sbuffer_remove(buffer, &data, buffer_lock); // Remove node if read by all threads
        } else {
            id = data.id;
            temperature = data.value;
            timestamp = data.ts;
            current_node->read_by_storagemgr = 1;
        }

        int i = 0;
        double sum;

        // Get sensor with corresponding ID from the list
        current_sensor = dpl_get_sensor_with_id(sensor_list, id);
        if (current_sensor == NULL) {
            sprintf(msg, "Received sensor data with invalid sensor node %d.\n", id);
            write_into_log_pipe(msg);
        }

        if (current_sensor != NULL) {
            // Don't have 5 measurements yet
            if ((current_sensor->nrOfTemp) < RUN_AVG_LENGTH) {
                current_sensor->temperatures[(current_sensor->nrOfTemp)] = temperature;
                (current_sensor->nrOfTemp)++;
            }

            // Have more than 5 measurements
            if ((current_sensor->nrOfTemp) >= RUN_AVG_LENGTH) {
                current_sensor->temperatures[RUN_AVG_LENGTH - 1] = temperature;
                current_sensor->nrOfTemp++;

                // Re-compute average each time
                sum = 0;
                for (i = 0; i < RUN_AVG_LENGTH; i++) {
                    sum += current_sensor->temperatures[i];
                }

                double avg = sum / RUN_AVG_LENGTH;
                current_sensor->temp_avg = avg;

                if (avg < SET_MIN_TEMP) {
                    sprintf(msg, "Sensor node %d reports it’s too cold (avg temp = %f).\n", id, avg);
                    write_into_log_pipe(msg);
                }

                if (avg > SET_MAX_TEMP) {
                    sprintf(msg, "Sensor node %d reports it’s too hot (avg temp = %f).\n", id, avg);
                    write_into_log_pipe(msg);
                }

                current_sensor->last_modified = timestamp;

                for (i = 0; i < RUN_AVG_LENGTH; i++) {
                    if (i == RUN_AVG_LENGTH - 1) break;
                    current_sensor->temperatures[i] = current_sensor->temperatures[i + 1];
                }
            }

            // Have exactly 5 measurements, so compute the first average
            if ((current_sensor->nrOfTemp) == RUN_AVG_LENGTH - 1) {
                sum = 0;
                for (i = 0; i < RUN_AVG_LENGTH; i++) {
                    sum += current_sensor->temperatures[i];
                }

                double avg = sum / RUN_AVG_LENGTH;
                current_sensor->temp_avg = avg;

                if (avg < SET_MIN_TEMP) {
                    sprintf(msg, "Sensor node %d reports it’s too cold (avg temp = %f).\n", id, avg);
                    write_into_log_pipe(msg);
                }

                if (avg > SET_MAX_TEMP) {
                    sprintf(msg, "Sensor node %d reports it’s too hot (avg temp = %f).\n", id, avg);
                    write_into_log_pipe(msg);
                }

                current_sensor->last_modified = timestamp;

                for (i = 0; i < RUN_AVG_LENGTH; i++) {
                    if (i == RUN_AVG_LENGTH - 1) break;
                    current_sensor->temperatures[i] = current_sensor->temperatures[i + 1];
                }
            }
        }

        current_node = current_node->next;
    }

    pthread_mutex_unlock(&buffer_lock);
    print_content(sensor_list);
}

void data_manager_free() {
    dpl_free(&sensor_list, true);
}
