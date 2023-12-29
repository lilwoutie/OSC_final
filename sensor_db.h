#ifndef SENSOR_DATABASE_H_
#define SENSOR_DATABASE_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#include "config.h"

/**
 * Creates or opens an existing file. If `append` is true, opens for append; if false, clears the file and overwrites.
 * Gets a pipe passed as a parameter to be able to write log messages.
 */
FILE *open_sensor_database(char *filename, bool append);

/**
 * Inserts data in CSV form in the file that's given as a parameter.
 * Gets a pipe passed as a parameter to be able to write log messages.
 */
int insert_sensor_data(FILE *file, sensor_id_t id, sensor_value_t value, sensor_ts_t ts);

/**
 * Closes the file and writes a log message into the pipe.
 */
int close_sensor_database(FILE *file);

#endif  // SENSOR_DATABASE_H_
