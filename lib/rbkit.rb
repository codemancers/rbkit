require "rbkit_tracer"
require "rbkit/timer"
require "rbkit/rbkit_gc"
require "objspace"

# Class implements user friendly interface in pure Ruby for profiler.
module Rbkit
  DEFAULT_PUB_PORT = 5555
  DEFAULT_REQ_PORT = 5556

  class Profiler
    attr_accessor :pub_port, :request_port

    def initialize(pub_port, request_port)
      [pub_port, request_port].each{|port| validate_port_range(port) }
      @pub_port = pub_port
      @request_port = request_port
      @profiler_thread = nil
      @stop_thread = false
      @server_running = false
      @gc_stats_timer = Rbkit::Timer.new(5) do
        data = RbkitGC.stat
        Rbkit.send_hash_as_event(data, Rbkit::EVENT_TYPES[:gc_stats])
      end
      @message_dispatch_timer = Rbkit::Timer.new(1) do
        Rbkit.send_messages
      end
    end

    def start_server(enable_object_trace: false, enable_gc_stats: false)
      if @server_running || !Rbkit.start_stat_server(pub_port, request_port)
        $stderr.puts "Rbkit server couldn't bind to socket, check if it is already" \
          " running. Profiling data will not be available."
        return false
      end
      Rbkit.start_stat_tracing if enable_object_trace
      @enable_gc_stats = enable_gc_stats
      @server_running = true
      @profiler_thread = Thread.new do
        loop do
          break if @stop_thread
          incoming_request = Rbkit.poll_for_request
          process_incoming_request(incoming_request)
          @gc_stats_timer.run if @enable_gc_stats
          @message_dispatch_timer.run
          # Let us sleep this thread for a bit, so as other things can run.
          sleep(0.05)
        end
      end
      at_exit { make_clean_exit(exiting: true) }
      true
    end

    def process_incoming_request(incoming_request)
      case incoming_request
      when "start_memory_profile"
        Rbkit.start_stat_tracing
        @enable_gc_stats = true
      when "stop_memory_profile"
        Rbkit.stop_stat_tracing
        @enable_gc_stats = false
      when "trigger_gc"
        GC.start
      when "objectspace_snapshot"
        Rbkit.send_objectspace_dump
      end
    end

    def stop_server
      Rbkit.stop_stat_server
    end

    def make_clean_exit(exiting: false)
      return false if !@server_running
      @stop_thread = true
      stop_server
      @server_running = false
      true
    end

    private

    def validate_port_range(port)
      raise ArgumentError, 'Invalid port value' unless (1024..65000).include?(port)
    end
  end

  ########### Rbkit API ###########

  # Starts the Rbkit server and waits for a client to connect and issue
  # commands to the request_port, until then there's zero performance overhead.
  # Profiling data is sent asynchronously over pub_port.
  # This method can be called early in a ruby application so that
  # whenever profiling needs to be done, the client can attach itself to the
  # inactive server, do the profiling and leave.
  def self.start_server(pub_port: DEFAULT_PUB_PORT, request_port: DEFAULT_REQ_PORT)
    @profiler ||= Rbkit::Profiler.new(pub_port, request_port)
    @profiler.start_server
  end

  # Starts the server with all tracepoints enabled by default. User can
  # optionally disable tracepoints using the optional arguments.
  # This method can be used to profile the startup process of a ruby
  # application where sending commands from the client to enable
  # profiling is not feasible.
  def self.start_profiling(pub_port: DEFAULT_PUB_PORT, request_port: DEFAULT_REQ_PORT,
                          enable_object_trace: true, enable_gc_stats: true)
    @profiler ||= Rbkit::Profiler.new(pub_port, request_port)
    @profiler.start_server(enable_object_trace: enable_object_trace,
                           enable_gc_stats: enable_gc_stats)
  end

  # Stops profiling and brings down the rbkit server if it's running
  def self.stop_server
    if !@profiler.nil? && @profiler.make_clean_exit
      @profiler = nil
      true
    else
      $stderr.puts "Cannot stop Rbkit server. Is it running?"
      false
    end
  end
end
