require 'msgpack'

message = File.read("/Users/yuva/nomads/rbkit-client/tests/msgpack/objcreated")
unpacked_message = MessagePack.unpack(message)

dump = MessagePack.unpack(File.read("/Users/yuva/00013"))
unpacked_message['payload'] = [dump]


File.open("/Users/yuva/00013m", "w") do |f|
  f.write MessagePack.pack(unpacked_message)
end
