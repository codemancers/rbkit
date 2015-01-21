require 'spec_helper'
require 'support/have_message_matcher'
require 'msgpack'

describe "Objectspace dump" do
  let(:payload) { Rbkit::MESSAGE_FIELDS[:payload] }
  let(:event_type) { Rbkit::MESSAGE_FIELDS[:event_type] }
  let(:object_id) { Rbkit::MESSAGE_FIELDS[:object_id] }
  let(:class_name) { Rbkit::MESSAGE_FIELDS[:class_name] }
  let(:file) { Rbkit::MESSAGE_FIELDS[:file] }
  let(:line) { Rbkit::MESSAGE_FIELDS[:line] }
  let(:size_field) { Rbkit::MESSAGE_FIELDS[:size] }
  let(:references) { Rbkit::MESSAGE_FIELDS[:references] }
  let(:object_dump_messages) do
    @message_list[payload]
      .select{|x| x[event_type] == Rbkit::EVENT_TYPES[:object_space_dump]}
  end
  let(:object_count) do
    object_dump_messages.inject(0){|sum, x| (sum + x[payload].size) }
  end
  let(:foo_info) do
    object_dump_messages.collect_concat do |message|
      message[payload].select{|x| x[class_name] == 'Foo'}
    end
  end
  let(:bar_info) do
    object_dump_messages.collect_concat do |message|
      message[payload].select{|x| x[class_name] == 'Bar'}
    end
  end
  let(:short_lived_bar_info) do
    object_dump_messages.collect_concat do |message|
      message[payload].select{|x| x[class_name] == 'ShortLivedBar'}
    end
  end
  let(:array_info) do
    object_dump_messages.collect_concat do |message|
      message[payload].select{|x| x[object_id] == foo_info.first[references].last}
    end
  end
  before(:all) do
    Rbkit.start_profiling(enable_gc_stats: false, enable_object_trace: true)
    @foo_obj_line = __LINE__ + 1
    @foo_obj = Foo.new
    Rbkit.send_objectspace_dump
    packed_message = Rbkit.get_queued_messages
    Rbkit.stop_server
    @message_list  = MessagePack.unpack packed_message
  end

  it "should be part of message list" do
    expect(@message_list)
      .to have_message(Rbkit::EVENT_TYPES[:object_space_dump])
  end

  it 'should be split into messages of 1000 objects each' do
    message_count, left_over_objects = object_count.divmod(1000)
    message_count += 1 unless left_over_objects.zero?
    expect(object_dump_messages.size).to eql(message_count)
  end

  it 'should record objects only once' do
    expect(foo_info.size).to eql 1
    expect(bar_info.size).to eql 1
    expect(short_lived_bar_info.size).to eql 1
    expect(array_info.size).to eql 1
  end

  it 'should record correct file info' do
    expect(foo_info.first[file]).to eql __FILE__
    expect(bar_info.first[file]).to eql foo_bar_source_file
    expect(array_info.first[file]).to eql foo_bar_source_file
  end

  it 'should record correct line info' do
    expect(foo_info.first[line]).to eql @foo_obj_line
    expect(bar_info.first[line]).to eql bar_obj_line
    expect(array_info.first[line]).to eql array_line
  end

  it 'should record correct references' do
    expect(foo_info.first[object_id]).to eql @foo_obj.object_id
    expect(bar_info.first[object_id]).to eql @foo_obj.bar.object_id
    expect(array_info.first[object_id]).to eql @foo_obj.array.object_id
  end

  it 'should record correct references' do
    expect(foo_info.first[references]).to include(bar_info.first[object_id])
    expect(foo_info.first[references]).to include(array_info.first[object_id])
  end

  it 'should record correct size' do
    expect(array_info.first[size_field]).to be > 0
  end
end
