require "zmq"
require "pp"
require "thread"

$ctx = ZMQ::Context.new
Thread.abort_on_exception = true

ENDPOINT = "tcp://127.0.0.1:5555"

class Server
  @@publish_topic = "foo"

  def start_publisher
    server_socket = $ctx.socket(:PUB)
    # server_socket.verbose = true
    server_socket.bind(ENDPOINT)
    server_socket.linger = 0
    loop do
      server_socket.sendm(@@publish_topic)
      server_socket.send("hello #{Time.now}")

      server_socket.sendm("lol")
      server_socket.send("for lol #{Time.now}")
      sleep(1)
    end
  end

  def start_listener
    listener_socket = $ctx.socket(:SUB)
    listener_socket.subscribe("bar")
    listener_socket.verbose = true
    listener_socket.connect(ENDPOINT)

    loop do
      data = listener_socket.recv
      puts "********** Received from client #{data}"
    end
  end
end

server = Server.new()
threads = []

threads << Thread.new do
  server.start_publisher
end

threads << Thread.new do
  server.start_listener
end

threads.each { |thr| thr.join }
