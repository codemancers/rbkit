require "rbkit_tracer"

# Top level module for profiler
module Rbkit
  def self.start_profiling(port = nil)
    start_server(port)
    start_stat_tracing
    at_exit { stop_server }
  end
end
