require 'spec_helper'
require 'support/have_message_matcher'
require 'msgpack'

describe "obj_destroyed event" do
  before(:all) do
    class Bar;end
    class Foo
      def initialize
        Bar.new
      end
    end

    Rbkit.start_profiling(enable_gc_stats: false, enable_object_trace: true)
    @foo_obj = Foo.new
    GC.start
    packed_message = Rbkit.get_queued_messages
    Rbkit.stop_server
    @message_list  = MessagePack.unpack packed_message
    @foo_info = @message_list['payload']
      .select{|x| x['event_type'] == 'obj_destroyed' && x['payload']['object_id'] == object_id_to_pointer_addr(@foo_obj.object_id)}
    bar_object_id = @message_list['payload']
      .find{|x| x['event_type'] == 'obj_created' && x['payload']['class'] == 'Bar'}['payload']['object_id']
    @bar_info = @message_list['payload']
      .select{|x| x['event_type'] == 'obj_destroyed' && x['payload']['object_id'] == bar_object_id}
  end
  it "should be part of message list" do
    expect(@message_list).to have_message('obj_destroyed')
  end

  it 'should record the deleted object' do
    expect(@bar_info.size).to eql 1
  end

  it 'should not record the live object' do
    expect(@foo_info.size).to eql 0
  end
end


