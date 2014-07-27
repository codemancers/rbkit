# encoding: utf-8
# To use this client install following gems
# gem install msgpack
# gem install rbczmq

require 'zmq'
require 'pp'
require "msgpack"

# PUB / SUB topology

Thread.abort_on_exception = true
fl = File.open("foo.dat", "w")

ctx = ZMQ::Context.new
socket = ctx.socket(:SUB)
socket.verbose = true
socket.subscribe("")
socket.connect("tcp://localhost:5555")
loop do
  message = socket.recv
  unpacked_message = MessagePack.unpack(message)
  p unpacked_message
end
fl.close()
