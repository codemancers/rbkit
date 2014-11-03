require 'spec_helper'
require 'support/have_message_matcher'
require 'msgpack'

describe "Objectspace dump" do
  before(:all) do
    Rbkit.start_profiling(enable_gc_stats: false, enable_object_trace: true)
    @foo_obj_line = __LINE__ + 1
    @foo_obj = Foo.new
    Rbkit.send_objectspace_dump
    packed_message = Rbkit.get_queued_messages
    Rbkit.stop_server
    @message_list  = MessagePack.unpack packed_message
    @message = @message_list['payload']
      .find{|x| x['event_type'] == Rbkit::EVENT_TYPES[:object_space_dump]}
    @foo_info = @message['payload'].select{|x| x['class_name'] == 'Foo'}
    @bar_info = @message['payload'].select{|x| x['class_name'] == 'Bar'}
    @short_lived_bar_info = @message['payload'].select{|x| x['class_name'] == 'ShortLivedBar'}
    @array_info = @message['payload']
      .select{|obj| obj['object_id'] == @foo_info.first['references'].last }
  end
  it "should be part of message list" do
    expect(@message_list)
      .to have_message(Rbkit::EVENT_TYPES[:object_space_dump])
      .with_count(1)
  end

  it 'should record objects only once' do
    expect(@foo_info.size).to eql 1
    expect(@bar_info.size).to eql 1
    expect(@short_lived_bar_info.size).to eql 1
  end

  it 'should record correct file info' do
    expect(@foo_info.first['file']).to eql __FILE__
    expect(@bar_info.first['file']).to eql foo_bar_source_file
  end

  it 'should record correct line info' do
    expect(@foo_info.first['line']).to eql @foo_obj_line
    expect(@bar_info.first['line']).to eql bar_obj_line
    expect(@array_info.first['line']).to eql array_line
  end

  it 'should record correct references' do
    expect(@foo_info.first['references']).to include(@bar_info.first['object_id'])
    expect(@foo_info.first['references']).to include(@array_info.first['object_id'])
  end

  it 'should record correct size' do
    expect(@array_info.first['size']).to eql @foo_obj.array.size * 8
  end
end
