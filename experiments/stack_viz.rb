$:<< File.join(File.dirname(__FILE__), "../lib")
$:<< File.join(File.dirname(__FILE__), "../ext")

require 'rbkit'

def costly_method
  5000000.times do
  end
end

def fibonacci( n )
  return  n  if ( 0..1 ).include? n
  ( fibonacci( n - 1 ) + fibonacci( n - 2 ) )
end

def do_small_stuff
  5000.times do
  end
end

def do_something
  something = 100
end

def do_costly_stuff
  costly_method
end

def main
  do_small_stuff
  do_costly_stuff
  method_that_sleeps()
  fibonacci(5)
end

def method_that_sleeps
  sleep 5
end

Rbkit.start_profiling(enable_object_trace: false, enable_gc_stats: false)

main()
do_costly_stuff

Rbkit.stop_server
