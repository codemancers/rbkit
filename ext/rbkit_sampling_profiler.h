#ifndef RBKIT_SAMPLING_PROFILER
#define RBKIT_SAMPLING_PROFILER
#include <stddef.h>
#include "rbkit_stack_trace.h"

typedef struct _rbkit_cpu_sample {
  size_t frame_count;
  rbkit_frame_data *frames;
  double timestamp;
} rbkit_cpu_sample;

typedef void (*queue_sample_func_ptr) (rbkit_cpu_sample *);

void rbkit_install_sampling_profiler(int wall_time, int interval, queue_sample_func_ptr func);
void rbkit_uninstall_sampling_profiler();
#endif
