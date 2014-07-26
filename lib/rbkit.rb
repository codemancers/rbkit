require "rbkit_tracer"

module Rbkit
  class Profiler
    attr_accessor :pub_port, :request_port

    def initialize(pub_port, request_port)
      @pub_port = pub_port
      @request_port = request_port
      @profiler_thread = nil
    end

    def start_server
      @profiler_thread = Thread.new do
        Rbkit.start_server(pub_port, request_port)
        loop do
          incoming_request = Rbkit.poll_for_request
          process_incoming_request(incoming_request)
        end
      end
    end

    def process_incoming_request(incoming_request)
      case incoming_request
      when "start_memory_profile"
        Rbkit.start_stat_tracing
      when "stop_memory_profile"
        Rbkit.stop_stat_tracing
      when "heap_snapshot"
        puts "Not implemented"
      end
    end
  end

  def self.start_profiling(pub_port = nil, request_port = nil)
    start_server(pub_port, request_port)
    start_stat_tracing
    at_exit { stop_server }
  end
end
