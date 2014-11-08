require 'spec_helper'
require 'msgpack'

describe 'send_hash_as_event' do
  let(:hash) { {'foo' => 'bar', 123 => "hello world"} }
  let(:payload) { Rbkit::MESSAGE_FIELDS[:payload] }
  let(:event_type) { Rbkit::MESSAGE_FIELDS[:event_type] }
  describe 'when event_type is known' do
    before do
      Rbkit.start_profiling(enable_gc_stats: false, enable_object_trace: false)
      Rbkit.send_hash_as_event(hash, Rbkit::EVENT_TYPES[:gc_stats])
      packed_message = Rbkit.get_queued_messages
      @message = MessagePack.unpack packed_message
      Rbkit.stop_server
    end
    it 'should create a custom event with the serialized hash' do
      expect(@message[payload].first[event_type]).to eql Rbkit::EVENT_TYPES[:gc_stats]
      expect(@message[payload].first[payload]).to eql hash
    end
  end

  describe 'when event_type is not known' do
    it 'should raise NotImplementedError with a meaningful message' do
      expect { Rbkit.send_hash_as_event(hash, 100) }
        .to raise_error(NotImplementedError,
          "Rbkit : Unpacking of event type '100' not implemented")
    end
  end
end
