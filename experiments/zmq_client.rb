require "zmq"
require "pp"

$ctx = ZMQ::Context.new

ENDPOINT = "tcp://127.0.0.1:5555"
Thread.abort_on_exception = true

class Client
  @@publish_topic = "bar"

  def start_publisher
    pub_socket = $ctx.socket(:PUB)
    pub_socket.verbose = true
    pub_socket.connect(ENDPOINT)
    loop do
      pub_socket.send("#{@@publish_topic} client#{Time.now}")
      sleep(1)
    end
  end

  def start_subscriber
    sub_socket = $ctx.socket(:SUB)
    # sub_socket.verbose = true
    sub_socket.subscribe("foo")
    sub_socket.connect(ENDPOINT)
    loop do
      data = sub_socket.recv
      puts "********** Received from server #{data}"
    end
  end
end


client = Client.new()
threads = []

threads << Thread.new do
  client.start_publisher
end

threads << Thread.new do
  client.start_subscriber
end

threads.each { |thr| thr.join }
