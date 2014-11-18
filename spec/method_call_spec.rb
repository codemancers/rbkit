require 'spec_helper'
require 'support/have_message_matcher'
require 'msgpack'

describe "method_call event" do
  def dummy_method
    __LINE__
  end
  let(:payload) { Rbkit::MESSAGE_FIELDS[:payload] }
  let(:event_type) { Rbkit::MESSAGE_FIELDS[:event_type] }
  let(:method_name) { Rbkit::MESSAGE_FIELDS[:method_name] }
  let(:file) { Rbkit::MESSAGE_FIELDS[:file] }
  let(:line) { Rbkit::MESSAGE_FIELDS[:line] }
  let(:thread_id) { Rbkit::MESSAGE_FIELDS[:thread_id] }
  let(:method_data) do
    @message_list[payload]
      .select{|x| x[event_type] == Rbkit::EVENT_TYPES[:method_call] &&
        x[payload][file] == __FILE__ }
  end
  before(:all) do
    Rbkit.start_profiling(enable_gc_stats: false, enable_object_trace: false,
                         enable_execution_trace: true)
    @line = dummy_method
    packed_message = Rbkit.get_queued_messages
    Rbkit.stop_server
    @message_list  = MessagePack.unpack packed_message
  end
  it "should be part of message list" do
    expect(@message_list).to have_message(Rbkit::EVENT_TYPES[:method_call])
  end
  it 'should trace method call only once' do
    expect(method_data.size).to eql 1
  end
  it 'should record correct method name' do
    expect(method_data.first[payload][method_name]).to eql 'dummy_method'
  end
  it 'should record correct method file' do
    expect(method_data.first[payload][file]).to eql __FILE__
  end
  it 'should record correct method line' do
    expect(method_data.first[payload][line]).to eql @line - 1
  end
  it 'should record correct thread id' do
    expect(method_data.first[payload][thread_id]).to eql Thread.current.object_id
  end
end



