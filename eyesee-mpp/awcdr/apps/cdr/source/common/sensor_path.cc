/* *******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 * *******************************************************************************/

#include "common/sensor_path.h"

#include <fcntl.h>

#include "common/app_log.h"

int ReadSensor(READABLE_SCNSOR scnsor) {
  int result = -1;
  const char* path = sensor_path(scnsor);
  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    db_error("(%s) open Fail!", path);
    return result;
  }
  char buffer[32];
  int count = read(fd, buffer, sizeof(buffer));
  if (count < 0) {
    db_error("(%s) read Fail!", path);
    close(fd);
    return result;
  }
  sscanf(buffer, "%d", &result);
  close(fd);
  return result;
}
