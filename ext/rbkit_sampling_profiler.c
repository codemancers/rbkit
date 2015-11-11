#include "rbkit_sampling_profiler.h"
#include "stdio.h"
#include "string.h"
#include "ruby/ruby.h"
#include "ruby/debug.h"
#include <signal.h>
#include <sys/time.h>
#include "rbkit_time_helper.h"
#include "rbkit_stack_trace.h"

#define BUF_SIZE 2048

static int signal_type;
static int clock_type;
queue_sample_func_ptr queue_cpu_sample_for_sending;

#ifdef RBKIT_DEV
static double total_wall_time_spent_in_sampling;
static double total_cpu_time_spent_in_sampling;
#endif


static void sampling_job_handler(void *data_unused) {
  rbkit_cpu_sample *sample = malloc(sizeof(rbkit_cpu_sample));
  rbkit_stack_trace *stacktrace = malloc(sizeof(rbkit_stack_trace));

  collect_stack_trace(stacktrace);
  sample->frames = stacktrace->frames;
  sample->frame_count = stacktrace->frame_count;
  queue_cpu_sample_for_sending(sample);
  delete_stack_trace(stacktrace);
  free(sample);
}

static void signal_handler(int signal, siginfo_t* sinfo, void* ucontext) {
#ifdef RBKIT_DEV
  double start_wall_time = get_wall_time_in_usec();
  double start_cpu_time = get_cpu_time_in_usec();
#endif
  rb_postponed_job_register_one(0, sampling_job_handler, 0);
#ifdef RBKIT_DEV
  total_wall_time_spent_in_sampling += (get_wall_time_in_usec() - start_wall_time);
  total_cpu_time_spent_in_sampling += (get_cpu_time_in_usec() - start_cpu_time);
#endif
  return;
}

static void install_signal_handler() {
  struct sigaction sa;
#ifdef RBKIT_DEV
  total_cpu_time_spent_in_sampling = total_wall_time_spent_in_sampling = 0;
#endif
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
#ifdef RBKIT_DEV
  fprintf(stderr, "Time spent in CPU sampling in usecs = %f(wall) %f(cpu)\n", total_wall_time_spent_in_sampling, total_cpu_time_spent_in_sampling);
#endif
  return;
}

static void start_sigprof_timer(int interval) {
  struct itimerval timer;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = interval;
  timer.it_value = timer.it_interval;
  setitimer(clock_type, &timer, 0);
}

static void stop_sigprof_timer() {
  struct itimerval timer;
  memset(&timer, 0, sizeof(timer));
  setitimer(clock_type, &timer, 0);
}

void rbkit_install_sampling_profiler(int wall_time, int interval, queue_sample_func_ptr func) {
  queue_cpu_sample_for_sending = func;
  if(wall_time) {
    signal_type = SIGALRM;
    clock_type = ITIMER_REAL;
  } else {
    signal_type = SIGPROF;
    clock_type = ITIMER_PROF;
  }
  install_signal_handler();
  start_sigprof_timer(interval);
}

void rbkit_uninstall_sampling_profiler() {
  stop_sigprof_timer();
  uninstall_signal_handler();
}
