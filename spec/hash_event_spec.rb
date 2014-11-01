require 'spec_helper'
require 'msgpack'

describe 'send_hash_as_event' do
  let(:hash) { {'foo' => 'bar', 123 => "hello world"} }
  before do
    Rbkit.start_profiling(enable_gc_stats: false, enable_object_trace: false)
    Rbkit.send_hash_as_event(hash, "custom_event_name")
    packed_message = Rbkit.get_queued_messages
    @message = MessagePack.unpack packed_message
    Rbkit.stop_server
  end
  it 'should create a custom event with the serialized hash' do
    expect(@message['payload'].first['event_type']).to eql 'custom_event_name'
    expect(@message['payload'].first['payload']).to eql hash
  end
end
