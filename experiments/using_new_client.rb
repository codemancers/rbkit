require 'zmq'
require 'pp'
require "msgpack"

Thread.abort_on_exception = true

ctx = ZMQ::Context.new
socket = ctx.socket(:SUB)
socket.subscribe("")
socket.connect("tcp://127.0.0.1:5555")

request_socket = ctx.socket(:REQ)
request_socket.connect("tcp://127.0.0.1:5556")
request_socket.send("start_memory_profile")
response = request_socket.recv()
puts "received #{response}"

loop do
  message = socket.recv
  unpacked_message = MessagePack.unpack(message)
  p unpacked_message
end
