#include "lib/timer.h"

#include <sys/time.h>

double now() {
  struct timeval time;
  gettimeofday(&time, 0);
  return double(time.tv_sec) + double(time.tv_usec) / 1000000.0;
}

double delta(double start) {
  return now() - start;
}
