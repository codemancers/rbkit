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
    puts "Commands : [start_memory_profile, stop_memory_profile]"
    command = gets.strip
    unless command.empty?
      request_socket.send(command)
      puts "sent #{command}"
      response = request_socket.recv()
      puts "received #{response}"
    end
  end
end

loop do
  message = socket.recv
  unpacked_message = MessagePack.unpack(message)
  puts "received #{unpacked_message.inspect}"
end
