require 'msgpack'

id   = 0
name = 1
refs = 2
file = 3
line = 4
size = 5


unpacked = MessagePack.unpack(File.read("/Users/yuva/00013-02-convert-id-to-int"))

new_objects = []
unpacked['payload'].first['payload'].map do |object|
  new_object = {
    id   => object['object_id'],
    name => object['class_name'],
    refs => object['references'],
    file => object['file'],
    line => object['line'],
    size => object['size']
  }

  new_objects << new_object
end


unpacked['payload'].first['payload'] = new_objects

File.open("/Users/yuva/00013-03-object-keys-to-enums", "w") do |f|
  f.write MessagePack.pack(unpacked)
end
