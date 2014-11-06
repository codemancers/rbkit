require 'msgpack'

unpacked = MessagePack.unpack(File.read("/Users/yuva/00013-01-inside-event-collection"))

unpacked['payload'].first['payload'].map do |object|
  object['object_id'] = object['object_id'].to_i(16)
end


File.open("/Users/yuva/00013-02-convert-id-to-int", "w") do |f|
  f.write MessagePack.pack(unpacked)
end
