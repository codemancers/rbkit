require 'spec_helper'
require 'msgpack'

describe 'timing of events' do
  def dummy_method
    sleep 2
  end
  let(:payload) { Rbkit::MESSAGE_FIELDS[:payload] }
  let(:event_type) { Rbkit::MESSAGE_FIELDS[:event_type] }
  let(:file) { Rbkit::MESSAGE_FIELDS[:file] }
  let(:cpu_time) { Rbkit::MESSAGE_FIELDS[:cpu_time] }
  let(:timestamp) { Rbkit::MESSAGE_FIELDS[:timestamp] }
  let(:method_call_data) do
    @message_list[payload]
      .select{|x| x[event_type] == Rbkit::EVENT_TYPES[:method_call] &&
        x[payload][file] == __FILE__ }
  end
  let(:method_return_data) do
    @message_list[payload]
      .select{|x| x[event_type] == Rbkit::EVENT_TYPES[:method_return] &&
        x[payload][file] == __FILE__ }
  end
  before(:all) do
    Rbkit.start_profiling(enable_gc_stats: false, enable_object_trace: false,
                         enable_execution_trace: true)
    dummy_method
    packed_message = Rbkit.get_queued_messages
    Rbkit.stop_server
    @message_list  = MessagePack.unpack packed_message
  end

  it 'should have a higher cpu time than method_call event' do
    expect(method_return_data.first[cpu_time])
      .to be > (method_call_data.first[cpu_time])
  end
  it 'should have a timestamp that is atleast 2 sec more than method_call event' do
    expect(method_return_data.first[timestamp] - method_call_data.first[timestamp])
      .to be_within(500).of (2 * 1000) # 2 secs, +/- 500ms
  end

  it 'cpu time should not consider time spent sleeping' do
    expect(method_return_data.first[cpu_time] - method_call_data.first[cpu_time])
      .to be_within(500).of (0) # 0 sec, +/- 500ms
  end
end
