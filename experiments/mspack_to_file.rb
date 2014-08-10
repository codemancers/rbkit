require 'msgpack'

message = {
  'event_type' => 'gc_start',
  'timestamp' => Time.now.to_f
}

File.open("/tmp/msgpack.dat", "w") do |fl|
  fl.write(message.to_msgpack)
end
