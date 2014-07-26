require "rbkit_tracer"

# Class implements user friendly interface in pure Ruby for profiler.
module Rbkit
  class Profiler
    attr_accessor :pub_port, :request_port, :stop_thread

    def initialize(pub_port, request_port)
      @pub_port = pub_port
      @request_port = request_port
      @profiler_thread = nil
      @stop_thread = false
    end

    def start_server
      @profiler_thread = Thread.new do
        Rbkit.start_server(pub_port, request_port)
        loop do
          break if @stop_thread
          incoming_request = Rbkit.poll_for_request
          process_incoming_request(incoming_request)
          # Let us sleep this thread for a bit, so as other things can run.
          sleep(0.05)
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

    def stop_server
      Rbkit.stop_server
    end
  end

  def self.start_profiling(pub_port = nil, request_port = nil)
    @profiler = Rbkit::Profiler.new(pub_port, request_port)
    @profiler.start_server
    at_exit do
      @profiler.stop_thread = true
      @profiler.stop_server
    end
  end

  def self.dump_objects
    send_objectspace_dump
  end
end
