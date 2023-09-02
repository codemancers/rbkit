#ifndef RBKIT_SAMPLING_PROFILER
#define RBKIT_SAMPLING_PROFILER
#include <stddef.h>

typedef struct _rbkit_frame_data {
  const char * method_name;
  const char * label;
  const char * file;
  unsigned long line;
  int is_singleton_method;
  unsigned long thread_id;
} rbkit_frame_data;

typedef struct _rbkit_cpu_sample {
  size_t frame_count;
  rbkit_frame_data *frames;
  double timestamp;
} rbkit_cpu_sample;

typedef void (*queue_sample_func_ptr) (rbkit_cpu_sample *);

void rbkit_install_sampling_profiler(int wall_time, int interval, int depth, queue_sample_func_ptr func);
void rbkit_uninstall_sampling_profiler();
#endif
