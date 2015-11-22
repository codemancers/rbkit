#include "ruby/ruby.h"
#include "ruby/debug.h"
#include "rbkit_stack_trace.h"

#define BUF_SIZE 2048

rbkit_stack_trace *collect_stack_trace() {
  int start = 0;
  int lines[BUF_SIZE];
  VALUE buff[BUF_SIZE];
  VALUE rb_method_name, rb_label, rb_file, rb_singleton_method;
  int i, is_singleton;
  char *method_name, *label, *file;
  unsigned long line, thread_id;

  int collected_size = rb_profile_frames(start, sizeof(buff) / sizeof(VALUE), buff, lines);
  rbkit_frame_data *frame_data = malloc(sizeof(rbkit_frame_data) * collected_size);
  rbkit_stack_trace *stacktrace = malloc(sizeof(rbkit_stack_trace));
  stacktrace->frames = frame_data;
  stacktrace->frame_count = collected_size;

  rb_gc_disable();
  for (i=0; i<collected_size; i++) {
    rb_method_name = rb_profile_frame_method_name(buff[i]);
    if(NIL_P(rb_method_name)) {
      method_name = NULL;
    } else {
      method_name = StringValueCStr(rb_method_name);
    }
    frame_data[i].method_name = method_name;

    rb_label = rb_profile_frame_full_label(buff[i]);
    label = StringValueCStr(rb_label);
    frame_data[i].label = label;

    rb_file = rb_profile_frame_absolute_path(buff[i]);
    if(NIL_P(rb_file))
      rb_file = rb_profile_frame_path(buff[i]);
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
  rb_gc_enable();
  return stacktrace;
}

void delete_stack_trace(rbkit_stack_trace *stacktrace) {
  free(stacktrace->frames);
  free(stacktrace);
}
