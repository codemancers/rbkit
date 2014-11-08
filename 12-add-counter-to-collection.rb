require 'msgpack'

file = "/Users/yuva/nomads/rbkit-client/tests/msgpack/hugedump"

message = File.read(file)
p unpacked_message = MessagePack.unpack(message)

unpacked_message[9] = rand(100)


File.open(file, "w") do |f|
  f.write MessagePack.pack(unpacked_message)
end
