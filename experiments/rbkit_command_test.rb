require 'zmq'
require 'msgpack'
require 'pp'

Thread.abort_on_exception = true


commands = [
  'start_memory_profile',
  'stop_memory_profile',
  'objectspace_snapshot',
  'trigger_gc'
]

output_file = File.open("/tmp/rbkit.log", "w")
puts "Writing output to file #{output_file.path}"
ctx = ZMQ::Context.new

puts "Enter IPv4 address of Rbkit server. (Blank for localhost) :"
server_ip = gets.strip
server_ip = "127.0.0.1" if server_ip.empty?

Thread.new do
  request_socket = ctx.socket(:REQ)
  request_socket.connect("tcp://#{server_ip}:5556")
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
socket.connect("tcp://#{server_ip}:5555")

begin
  index = 0
  loop do
    message = socket.recv

    unpacked_message = MessagePack.unpack(message)
    PP.pp(unpacked_message, output_file)
    output_file.flush

    already_got = []
    payload = unpacked_message['payload'].dup
    payload.each do |event|
      next if already_got.include?(event['event_type'])

      already_got << event['event_type']
      unpacked_message['payload'] = [event]

      filename = "%05d" % index
      File.open("/tmp/rbkitevents/#{filename}", "w") do |f|
        f.write MessagePack.pack(unpacked_message)
      end

      index = index + 1
    end

  end
ensure
  output_file.close
end
