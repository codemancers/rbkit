require "rbkit_tracer"
require "rbkit/timer"
require "rbkit/rbkit_gc"
require "objspace"

# Class implements user friendly interface in pure Ruby for profiler.
module Rbkit
  class Profiler
    attr_accessor :pub_port, :request_port

    def initialize(pub_port, request_port)
      @pub_port = pub_port
      @request_port = request_port
      @profiler_thread = nil
      @stop_thread = false
      @server_running = false
      @gc_stats_timer = Rbkit::Timer.new(5) do
        data = RbkitGC.stat
        Rbkit.send_hash_as_event(data, "gc_stats")
      end
      @message_dispatch_timer = Rbkit::Timer.new(1) do
        Rbkit.send_messages
      end
    end

    def start_server(enable_profiling: false)
      return if @server_running
      @profiler_thread = Thread.new do
        Rbkit.start_stat_server(pub_port, request_port)
        Rbkit.start_stat_tracing if enable_profiling
        loop do
          break if @stop_thread
          incoming_request = Rbkit.poll_for_request
          process_incoming_request(incoming_request)
          @gc_stats_timer.run
          @message_dispatch_timer.run
          # Let us sleep this thread for a bit, so as other things can run.
          sleep(0.05)
        end
      end
      @server_running = true
    end

    def process_incoming_request(incoming_request)
      case incoming_request
      when "start_memory_profile"
        Rbkit.start_stat_tracing
      when "stop_memory_profile"
        Rbkit.stop_stat_tracing
      when "trigger_gc"
        GC.start
      when "objectspace_snapshot"
        Rbkit.send_objectspace_dump
      end
    end

    def stop_server
      return if !@server_running
      Rbkit.stop_stat_server
      @server_running = false
    end

    def make_clean_exit
      @stop_thread = true
      stop_server
    end
  end

  ########### Rbkit API ###########

  # Starts the server and enables memory profiling tracepoints
  def self.start_profiling(pub_port = nil, request_port = nil)
    @profiler = Rbkit::Profiler.new(pub_port, request_port)
    @profiler.start_server(enable_profiling: true)
    at_exit do
      self.stop_server
    end
  end

  # Just starts the server and waits for instructions.
  # The client needs to connect to the request_port and send
  # commands over the wire. The client also needs to connect
  # to the pub_port and subscribe to the responses from the server.
  def self.start_server(pub_port = nil, request_port = nil)
    @profiler = Rbkit::Profiler.new(pub_port, request_port)
    @profiler.start_server
    at_exit do
      self.stop_server
    end
  end

  # Stops profiling and brings down the rbkit server if it's running
  def self.stop_server
    @profiler.make_clean_exit
  end
end
