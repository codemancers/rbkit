require 'zmq'
require "msgpack"

Thread.abort_on_exception = true

ctx = ZMQ::Context.new
socket = ctx.socket(:SUB)
socket.subscribe("")
socket.connect("tcp://127.0.0.1:5555")

Thread.new do
  request_socket = ctx.socket(:REQ)
  request_socket.connect("tcp://127.0.0.1:5556")
  loop do
    puts "Commands : [start_memory_profile, stop_memory_profile, objectspace_snapshot]"
    command = gets.strip
    unless command.empty?
      request_socket.send(command)
      puts "sent #{command}"
      response = request_socket.recv()
      puts "received #{response}"
    end
  end
end

File.open("/tmp/foo.dat", "a") do |fl|
  loop do
    message = socket.recv
    unpacked_message = MessagePack.unpack(message)
    fl.puts unpacked_message
  end
end
