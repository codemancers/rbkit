require 'msgpack'

message = File.read("/Users/yuva/nomads/rbkit-client/tests/msgpack/objcreated")
p unpacked_message = MessagePack.unpack(message)

dump = MessagePack.unpack(File.read("/Users/yuva/nomads/rbkit-client/tests/msgpack/objcreated"))
unpacked_message['payload'] = [dump]


File.open("/Users/yuva/nomads/rbkit-client/tests/msgpack/objcreated", "w") do |f|
  f.write MessagePack.pack(unpacked_message)
end
