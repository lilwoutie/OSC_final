#include "sbuffer.h"

//bool done = false; // initially not done with data

int sensor_buffer_init(sensor_buffer_t **sensor_buffer, pthread_mutex_t buffer_lock) {
    *sensor_buffer = malloc(sizeof(sensor_buffer_t));
    if (*sensor_buffer == NULL) return SBUFFER_FAILURE;
    (*sensor_buffer)->head = NULL;
    (*sensor_buffer)->tail = NULL;

    // Mutex initialization
    if (pthread_mutex_init(&buffer_lock, NULL) != 0) {
        printf("\nMutex initialization failed\n");
    }

    return SBUFFER_SUCCESS;
}

int sensor_buffer_free(sensor_buffer_t **sensor_buffer) {
    sbuffer_node_t *dummy;
    if ((sensor_buffer == NULL) || (*sensor_buffer == NULL)) {
        return SBUFFER_FAILURE;
    }
    while ((*sensor_buffer)->head) {
        dummy = (*sensor_buffer)->head;
        (*sensor_buffer)->head = (*sensor_buffer)->head->next;
        free(dummy);
    }
    free(*sensor_buffer);
    *sensor_buffer = NULL;
    return SBUFFER_SUCCESS;
}

int sensor_buffer_remove(sensor_buffer_t *sensor_buffer, sensor_data_t *data, pthread_mutex_t buffer_lock) {
    sbuffer_node_t *dummy;
    if (sensor_buffer == NULL) {
        return SBUFFER_FAILURE;
    }

    if (sensor_buffer->head == NULL) {
        return SBUFFER_NO_DATA;
    }

    *data = sensor_buffer->head->data;
    dummy = sensor_buffer->head;
    if (sensor_buffer->head == sensor_buffer->tail) {
        sensor_buffer->head = sensor_buffer->tail = NULL;
    } else {
        sensor_buffer->head = sensor_buffer->head->next;
    }
    free(dummy);
    return SBUFFER_SUCCESS;
}

int sensor_buffer_insert(sensor_buffer_t *sensor_buffer, sensor_data_t *data, pthread_mutex_t buffer_lock, pthread_cond_t cond) {
    pthread_mutex_lock(&buffer_lock);
    sbuffer_node_t *dummy;
    if (sensor_buffer == NULL) {
        pthread_mutex_unlock(&buffer_lock);
        return SBUFFER_FAILURE;
    }

    dummy = malloc(sizeof(sbuffer_node_t));

    if (dummy == NULL) {
        pthread_mutex_unlock(&buffer_lock);
        return SBUFFER_FAILURE;
    }

    dummy->data = *data;
    dummy->next = NULL;
    dummy->read_by_datamgr = 0;
    dummy->read_by_storagemgr = 0;

    if (sensor_buffer->tail == NULL) {
        sensor_buffer->head = sensor_buffer->tail = dummy;
    } else {
        sensor_buffer->tail->next = dummy;
        sensor_buffer->tail = sensor_buffer->tail->next;
    }
    pthread_mutex_unlock(&buffer_lock);
    pthread_cond_signal(&cond);
    return SBUFFER_SUCCESS;
}
