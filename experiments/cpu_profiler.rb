require 'zmq'
require 'msgpack'
require 'pp'

Thread.abort_on_exception = true


commands = [
  'start_memory_profile',
  'stop_memory_profile',
  'start_cpu_profile',
  'stop_cpu_profile',
  'objectspace_snapshot',
  'trigger_gc'
]

output_file = File.open("/tmp/rbkit.log", "w")
puts "Writing output to file #{output_file.path}"
ctx = ZMQ::Context.new

puts "Enter IPv4 address of Rbkit server. (Blank for localhost) :"
server_ip = gets.strip
server_ip = "127.0.0.1" if server_ip.empty?

pub_port = (ENV['RBKIT_PUB_PORT'] || 5555).to_i
req_port = (ENV['RBKIT_REQ_PORT'] || 5556).to_i

Thread.new do
  request_socket = ctx.socket(:REQ)
  request_socket.connect("tcp://#{server_ip}:#{req_port}")
  loop do
    puts "Available commands :"
    commands.each_with_index do |c, i|
      puts "#{i+1}. #{c}"
    end
    command = commands[gets.strip.to_i - 1] rescue ''
    unless command.empty?
      request_socket.send(command)
      puts "sent #{command}"
      response = request_socket.recv()
      puts "received #{response}"
    end
  end
end

socket = ctx.socket(:SUB)
socket.subscribe("")
socket.connect("tcp://#{server_ip}:#{pub_port}")

begin
  loop do
    message = socket.recv
    unpacked_message = MessagePack.unpack(message)
    PP.pp(unpacked_message, output_file)
    output_file.flush
  end
ensure
  output_file.close
end
