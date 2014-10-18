require 'spec_helper'
require 'support/have_message_matcher'
require 'msgpack'

describe "obj_created event" do
  before(:all) do
    Rbkit.start_profiling(enable_gc_stats: false, enable_object_trace: true)
    @foo_obj = Foo.new
    packed_message = Rbkit.get_queued_messages
    Rbkit.stop_server
    @message_list  = MessagePack.unpack packed_message
    @foo_info = @message_list['payload']
      .select{|x| x['event_type'] == 'obj_created' && x['payload']['class'] =='Foo' }
    @bar_info = @message_list['payload']
      .select{|x| x['event_type'] == 'obj_created' && x['payload']['class'] =='Bar' }
    @short_lived_bar_info = @message_list['payload']
      .select{|x| x['event_type'] == 'obj_created' && x['payload']['class'] =='ShortLivedBar' }
  end
  it "should be part of message list" do
    expect(@message_list).to have_message('obj_created')
  end

  it 'should record objects only once' do
    expect(@foo_info.size).to eql 1
    expect(@bar_info.size).to eql 1
    expect(@short_lived_bar_info.size).to eql 1
  end

  it 'should record correct object_id' do
    expect(pointer_addr_to_object_id(@foo_info.first['payload']['object_id']))
      .to eql @foo_obj.object_id
    expect(pointer_addr_to_object_id(@bar_info.first['payload']['object_id']))
      .to eql @foo_obj.bar.object_id
  end

  it 'should record correct class' do
    expect(@foo_info.first['payload']['class']).to eql @foo_obj.class.to_s
    expect(@bar_info.first['payload']['class']).to eql @foo_obj.bar.class.to_s
  end
end

