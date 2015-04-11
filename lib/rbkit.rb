require "rbkit/version"
require "rbkit_server"
require "rbkit/timer"
require "rbkit/rbkit_gc"
require "rbkit/server"
#require "objspace"

# Class implements user friendly interface in pure Ruby for profiler.
module Rbkit
  DEFAULT_PUB_PORT = 5555
  DEFAULT_REQ_PORT = 5556


  ########### Rbkit API ###########

  # Starts the Rbkit server and waits for a client to connect and issue
  # commands to the request_port, until then there's zero performance overhead.
  # Profiling data is sent asynchronously over pub_port.
  # This method can be called early in a ruby application so that
  # whenever profiling needs to be done, the client can attach itself to the
  # inactive server, do the profiling and leave.
  def self.start_server(pub_port: DEFAULT_PUB_PORT, request_port: DEFAULT_REQ_PORT)
    @server ||= Rbkit::Server.new(pub_port, request_port)
    @server.start
  end

  # Starts the server with all tracepoints enabled by default. User can
  # optionally disable tracepoints using the optional arguments.
  # This method can be used to profile the startup process of a ruby
  # application where sending commands from the client to enable
  # profiling is not feasible.
  def self.start_profiling(pub_port: DEFAULT_PUB_PORT, request_port: DEFAULT_REQ_PORT,
                          enable_object_trace: true, enable_gc_stats: true)
    @server ||= Rbkit::Server.new(pub_port, request_port)
    @server.start(enable_object_trace: enable_object_trace,
                           enable_gc_stats: enable_gc_stats)
  end

  # Stops profiling and brings down the rbkit server if it's running
  def self.stop_server
    if !@server.nil? && @server.make_clean_exit
      @server = nil
      true
    else
      $stderr.puts "Cannot stop Rbkit server. Is it running?"
      false
    end
  end
end
