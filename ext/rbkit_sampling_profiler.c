#include "rbkit_sampling_profiler.h"
#include "stdio.h"
#include "string.h"
#include "ruby/ruby.h"
#include "ruby/debug.h"
#include <signal.h>
#include <sys/time.h>

static int signal_type;
static int clock_type;
queue_sample_func_ptr queue_cpu_sample_for_sending;

static void sampling_job_handler(void *data_unused) {
  int start = 0;
  int buff_size = 100;
  int lines[256];
  VALUE buff[256], rb_method_name, rb_label, rb_file, rb_singleton_method;
  int i, is_singleton;
  char *method_name, *label, *file;
  unsigned long line, thread_id;

  rbkit_cpu_sample *sample = malloc(sizeof(rbkit_cpu_sample));

  int collected_size = rb_profile_frames(start, buff_size, buff, lines);
  rbkit_frame_data *frame_data = malloc(sizeof(rbkit_frame_data) * collected_size);
  sample->frames = frame_data;
  sample->frame_count = collected_size;
  for (i=0; i<collected_size; i++) {

    rb_method_name = rb_profile_frame_method_name(buff[i]);
    method_name = StringValueCStr(rb_method_name);
    frame_data[i].method_name = method_name;

    rb_label = rb_profile_frame_full_label(buff[i]);
    label = StringValueCStr(rb_label);
    frame_data[i].label = label;

    rb_file = rb_profile_frame_absolute_path(buff[i]);
    file = StringValueCStr(rb_file);
    frame_data[i].file = file;

    line = FIX2ULONG(rb_profile_frame_first_lineno(buff[i]));
    frame_data[i].line = line;

    rb_singleton_method = rb_profile_frame_singleton_method_p(buff[i]);
    is_singleton = rb_singleton_method == Qtrue;
    frame_data[i].is_singleton_method = is_singleton;

    thread_id = FIX2ULONG(rb_obj_id(rb_thread_current()));
    frame_data[i].thread_id = thread_id;
  }
  queue_cpu_sample_for_sending(sample);
  free(frame_data);
  free(sample);
}

static void signal_handler(int signal, siginfo_t* sinfo, void* ucontext) {
  rb_postponed_job_register_one(0, sampling_job_handler, 0);
  return;
}

static void install_signal_handler() {
  struct sigaction sa;
  sa.sa_sigaction = signal_handler;
  sa.sa_flags = SA_RESTART | SA_SIGINFO;
  sigemptyset(&sa.sa_mask);
  sigaction(signal_type, &sa, NULL);
}

static void uninstall_signal_handler() {
  struct sigaction sa;
  sa.sa_handler = SIG_IGN;
  sa.sa_flags = SA_RESTART;
  sigemptyset(&sa.sa_mask);
  sigaction(signal_type, &sa, NULL);
}

static void start_sigprof_timer() {
  struct itimerval timer;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 30000;
  timer.it_value = timer.it_interval;
  setitimer(clock_type, &timer, 0);
}

static void stop_sigprof_timer() {
  struct itimerval timer;
  memset(&timer, 0, sizeof(timer));
  setitimer(clock_type, &timer, 0);
}

void rbkit_install_sampling_profiler(int wall_time, queue_sample_func_ptr func) {
  queue_cpu_sample_for_sending = func;
  if(wall_time) {
    signal_type = SIGALRM;
    clock_type = ITIMER_REAL;
  } else {
    signal_type = SIGPROF;
    clock_type = ITIMER_PROF;
  }
  install_signal_handler();
  start_sigprof_timer();
}

void rbkit_uninstall_sampling_profiler() {
  stop_sigprof_timer();
  uninstall_signal_handler();
}
