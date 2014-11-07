require 'msgpack'

ek = {
  "event_type" => 0,
  "timestamp" => 1,
  "payload" => 2,
  "message_counter" => 9
}


def old2new(object)
  {
    3 => object['object_id'],
    4 => object['class_name'],
    5 => object['references'],
    6 => object['file'],
    7 => object['line'],
    8 => object['size']
  }.reject { |k, v| v.nil? }
end


file = "/Users/yuva/nomads/rbkit-client/tests/msgpack/gcstats"

message = File.read(file)
p unpacked_message = MessagePack.unpack(message)

unpacked = {}
unpacked_message.each do |k, v| # convert event-collection keys
  unpacked[ek[k]] = v
end

p unpacked

unpacked[2] = [{}]
unpacked_message['payload'].first.each do |k, v| # convert first event keys
  unpacked[2].first[ek[k]] = v
end

p unpacked

# unpacked[2].first[2] = unpacked[2].first[2].map { |o| old2new(o) }

# p unpacked


# unpacked_message['payload'].first['payload'].map do |k, v|
#   new_objects << new_object
# end


File.open(file, "w") do |f|
  f.write MessagePack.pack(unpacked)
end
