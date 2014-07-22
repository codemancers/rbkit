require "rbkit_tracer"

module Rbkit
  def self.start_profiling(port = nil)
    start_server(port)
    start_stat_tracing
    at_exit { stop_server }
  end

  def self.dump_objects
    send_objectspace_dump
  end
end
