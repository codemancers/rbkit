require 'msgpack'

message = File.read("/Users/yuva/nomads/rbkit-client/tests/msgpack/objdestroyed")
p unpacked_message = MessagePack.unpack(message)

unpacked_message['event_type'] = 7


File.open("/Users/yuva/nomads/rbkit-client/tests/msgpack/objdestroyed", "w") do |f|
  f.write MessagePack.pack(unpacked_message)
end
