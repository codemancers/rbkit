require "rbkit_tracer"

module Rbkit
  def self.start_profiling
    start_server
    start_stat_tracing
    at_exit { stop_server }
  end
end
