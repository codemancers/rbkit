require 'spec_helper'
require 'support/have_message_matcher'
require 'msgpack'

describe "obj_destroyed event" do
  let(:payload) { Rbkit::MESSAGE_FIELDS[:payload] }
  let(:event_type) { Rbkit::MESSAGE_FIELDS[:event_type] }
  let(:object_id) { Rbkit::MESSAGE_FIELDS[:object_id] }
  let(:class_name) { Rbkit::MESSAGE_FIELDS[:class_name] }
  let(:foo_info) do
    @message_list[payload]
      .select{|x| x[event_type] == Rbkit::EVENT_TYPES[:obj_destroyed] &&
        x[payload][object_id] == @foo_obj.object_id }
  end
  let(:short_lived_bar_info) do
    short_lived_bar_object_id = @message_list[payload]
      .find{|x| x[event_type] == Rbkit::EVENT_TYPES[:obj_created] &&
        x[payload][class_name] == 'ShortLivedBar'}[payload][object_id]
    @message_list[payload]
      .select{|x| x[event_type] == Rbkit::EVENT_TYPES[:obj_destroyed] &&
        x[payload][object_id] == short_lived_bar_object_id}
  end
  before(:all) do
    Rbkit.start_profiling(enable_gc_stats: false, enable_object_trace: true)
    @foo_obj = Foo.new
    GC.start
    packed_message = Rbkit.get_queued_messages
    Rbkit.stop_server
    @message_list  = MessagePack.unpack packed_message
  end
  it "should be part of message list" do
    expect(@message_list).to have_message(Rbkit::EVENT_TYPES[:obj_destroyed])
  end

  it 'should record the deleted object' do
    expect(short_lived_bar_info.size).to eql 1
  end

  it 'should not record the live object' do
    expect(foo_info.size).to eql 0
  end
end


