#include "rbkit_sampling_profiler.h"
#include "stdio.h"
#include "string.h"
#include "ruby/ruby.h"
#include "ruby/debug.h"

static int signal_type;
static int clock_type;

static void sampling_job_handler(void *data_unused) {
  int start = 0;
  int buff_size = 100;
  int lines[256];
  VALUE buff[256];
  int i;

  int collected_size = rb_profile_frames(start, buff_size, buff, lines);
  for (i=0; i<collected_size; i++) {
    VALUE rb_label = rb_profile_frame_label(buff[i]);
    char *label = StringValueCStr(rb_label);
    VALUE rb_file = rb_profile_frame_absolute_path(buff[i]);
    char *file = StringValueCStr(rb_file);
    unsigned long line = FIX2ULONG(rb_profile_frame_first_lineno(buff[i]));
    unsigned long thread_id = FIX2ULONG(rb_obj_id(rb_thread_current()));
    fprintf(stderr, "In Thread %lu: %s : %s:%lu\n", thread_id, label, file, line);
  }
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
  timer.it_interval.tv_usec = 300000;
  timer.it_value = timer.it_interval;
  setitimer(clock_type, &timer, 0);
}

static void stop_sigprof_timer() {
  struct itimerval timer;
  memset(&timer, 0, sizeof(timer));
  setitimer(clock_type, &timer, 0);
}

void rbkit_install_sampling_profiler(int wall_time) {
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

