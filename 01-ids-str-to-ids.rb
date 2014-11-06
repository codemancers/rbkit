require 'msgpack'

message = File.read("/Users/yuva/nomads/rbkit-client/tests/msgpack/objectdump")
p unpacked_message = MessagePack.unpack(message)
unpacked_message['payload'].map do |object|
  object['references'] = object['references'].map { |r| r.to_i(16) }
end


File.open("/Users/yuva/nomads/rbkit-client/tests/msgpack/objectdump", "w") do |f|
  f.write MessagePack.pack(unpacked_message)
end
