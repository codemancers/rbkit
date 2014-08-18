require 'mkmf'
if ENV['RBKIT_DEV']
  $stderr.puts "Dev environment enabled."
  $CFLAGS << ' -g'
end
if(have_func('rb_postponed_job_register_one') &&
  have_func('rb_profile_frames') &&
  have_func('rb_tracepoint_new') &&
  have_const('RUBY_INTERNAL_EVENT_NEWOBJ') &&
  have_library("zmq") &&
  have_header("zmq.h") &&
  have_library("msgpack") &&
  have_header("msgpack.h"))
  create_makefile('rbkit_test_helper')
else
  fail 'missing API: are you using ruby 2.1+?'
end
