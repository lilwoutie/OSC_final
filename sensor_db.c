#include "sensor_db.h"

char log_msg[LOG_MESSAGE_SIZE];

FILE *open_sensor_database(char *filename, bool append) {
    sprintf(log_msg, "A new CSV file is created or an existing file has been opened.\n");
    write_into_log_pipe(log_msg);

    if (append) {
        return fopen(filename, "a"); // Checks if the file exists; if not, a new file is created for appending
    } else {
        return fopen(filename, "w"); // If the file exists, its contents are destroyed; if not, it will be created
    }
}

int insert_sensor_data(FILE *file, sensor_id_t id, sensor_value_t value, sensor_ts_t ts) {
    if (file != NULL) {
        fprintf(file, "%d,%f,%ld,\n", id, value, ts);

        sprintf(log_msg, "Data insertion from sensor %d succeeded.\n", id);
        write_into_log_pipe(log_msg);

        return 0;
    } else {
        perror("Failed to open file");

        sprintf(log_msg, "An error occurred when writing to the CSV file.\n");
        write_into_log_pipe(log_msg);

        return 1;
    }
}

int close_sensor_database(FILE *file) {
    if (file != NULL) {
        fclose(file);
        sprintf(log_msg, "The CSV file has been closed.\n");
        write_into_log_pipe(log_msg);

        return 0;
    }
    return 1;
}
