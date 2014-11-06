require 'msgpack'

unpacked = MessagePack.unpack(File.read("/Users/yuva/00013-03-object-keys-to-enums"))

refs = 0
unpacked['payload'].first['payload'].each do |object|
  refs = refs + object[2].size
  object[2] = object[2].map { |r| r.to_i(16) }
end

p refs


File.open("/Users/yuva/00013-04-object-refs-to-ints", "w") do |f|
  f.write MessagePack.pack(unpacked)
end
