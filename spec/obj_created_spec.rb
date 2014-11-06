require 'spec_helper'
require 'support/have_message_matcher'
require 'msgpack'

describe "obj_created event" do
  let(:payload) { Rbkit::MESSAGE_FIELDS[:payload] }
  let(:class_name) { Rbkit::MESSAGE_FIELDS[:class_name] }
  let(:event_type) { Rbkit::MESSAGE_FIELDS[:event_type] }
  let(:object_id) { Rbkit::MESSAGE_FIELDS[:object_id] }
  let(:foo_info) do
    @message_list[payload]
      .select{|x| x[event_type] == Rbkit::EVENT_TYPES[:obj_created] &&
        x[payload][class_name] =='Foo' }
  end
  let(:bar_info) do
    @message_list[payload]
      .select{|x| x[event_type] == Rbkit::EVENT_TYPES[:obj_created] &&
        x[payload][class_name] =='Bar'}
  end
  let(:short_lived_bar_info) do
    @message_list[payload]
      .select{|x| x[event_type] == Rbkit::EVENT_TYPES[:obj_created] &&
        x[payload][class_name] =='ShortLivedBar' }
  end
  before(:all) do
    Rbkit.start_profiling(enable_gc_stats: false, enable_object_trace: true)
    @foo_obj = Foo.new
    packed_message = Rbkit.get_queued_messages
    Rbkit.stop_server
    @message_list  = MessagePack.unpack packed_message
  end
  it "should be part of message list" do
    expect(@message_list).to have_message(Rbkit::EVENT_TYPES[:obj_created])
  end

  it 'should record objects only once' do
    expect(foo_info.size).to eql 1
    expect(bar_info.size).to eql 1
    expect(short_lived_bar_info.size).to eql 1
  end

  it 'should record correct object_id' do
    expect(foo_info.first[payload][object_id]).to eql @foo_obj.object_id
    expect(bar_info.first[payload][object_id]).to eql @foo_obj.bar.object_id
  end

  it 'should record correct class_name' do
    expect(foo_info.first[payload][class_name]).to eql @foo_obj.class.to_s
    expect(bar_info.first[payload][class_name]).to eql @foo_obj.bar.class.to_s
  end
end

