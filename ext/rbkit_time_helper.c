#include <stdio.h>

#if defined(_WIN32)
double get_wall_time(){
  return 0; // Not supported on Windows
}
double get_cpu_time(){
  return 0; // Not supported on Windows
}

#else

#include <sys/time.h>

#if defined(__linux__)
#include <stdlib.h>
#include <time.h>

double get_cpu_time(){
  struct timespec clock;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID , &clock);
  return clock.tv_sec * 1000 + (clock.tv_nsec/1000000.0);
}

#elif defined(__APPLE__)
double get_cpu_time(){
    return (double)clock() * 1000.0 / CLOCKS_PER_SEC;
}
#endif

double get_wall_time(){
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000 + (tv.tv_usec/1000.0);
}
#endif
