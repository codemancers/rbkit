module Rbkit
  class Server
    attr_accessor :pub_port, :request_port

    def initialize(pub_port, request_port)
      @pub_port = pub_port.to_i
      @request_port = request_port.to_i
      [@pub_port, @request_port].each{|port| validate_port_range(port) }
      @profiler_thread = nil
      @profiler_stop_thread = false
      @server_running = false
      @clock_type = :cpu
      @gc_stats_timer = Rbkit::Timer.new(5) do
        data = RbkitGC.stat
        send_hash_as_event(data, Rbkit::EVENT_TYPES[:gc_stats])
      end
      @message_dispatch_timer = Rbkit::Timer.new(1) do
        send_messages
      end
    end

    def start(enable_object_trace: false, enable_gc_stats: false)
      if @server_running || !start_stat_server(pub_port, request_port)
        $stderr.puts "Rbkit server couldn't bind to socket, check if it is already" \
          " running. Profiling data will not be available."
        return false
      end
      start_stat_tracing if enable_object_trace
      @enable_gc_stats = enable_gc_stats
      @server_running = true
      @profiler_thread = Thread.new do
        loop do
          break if @stop_profiler_thread
          incoming_request = poll_for_request
          process_incoming_request(incoming_request)
          @gc_stats_timer.run if @enable_gc_stats
          @message_dispatch_timer.run
          # Let us sleep this thread for a bit, so as other things can run.
          sleep(0.05)
        end
      end
      at_exit { stop }
      self
    end

    def process_incoming_request(incoming_request)
      case incoming_request
      when "start_memory_profile"
        start_stat_tracing
        @enable_gc_stats = true
      when "stop_memory_profile"
        stop_stat_tracing
        @enable_gc_stats = false
      when "trigger_gc"
        GC.start
      when "objectspace_snapshot"
        send_objectspace_dump
      when "use_cpu_time"
        @clock_type = :cpu
      when "use_wall_time"
        @clock_type = :wall
      when "start_cpu_profiling"
        start_cpu_profiling(clock_type: @clock_type)
      when "stop_cpu_profiling"
        stop_cpu_profiling
      end
    end

    def stop
      return false if !@server_running
      @profiler_stop_thread = true
      stop_stat_server
      @server_running = false
      true
    end

    def start_cpu_profiling(mode: :sampling, clock_type: :cpu)
      @cpu_profiling_mode = mode
      if mode == :sampling
        start_sampling_profiler(clock_type)
      else
        # TODO
      end
    end

    def stop_cpu_profiling
      if @cpu_profiling_mode == :sampling
        stop_sampling_profiler
      else
        # TODO
      end
    end

    private

    def validate_port_range(port)
      raise ArgumentError, 'Invalid port value' unless (1024..65000).include?(port)
    end
  end
end

