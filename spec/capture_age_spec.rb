require 'spec_helper'
require 'support/have_message_matcher'
require 'msgpack'

describe "Capture object age" do
  let(:payload) { Rbkit::MESSAGE_FIELDS[:payload] }
  let(:object_dump) { Rbkit::EVENT_TYPES[:object_space_dump] }
  let(:event_type) { Rbkit::MESSAGE_FIELDS[:event_type] }
  let(:object_id) { Rbkit::MESSAGE_FIELDS[:object_id] }
  let(:class_name) { Rbkit::MESSAGE_FIELDS[:class_name] }
  let(:file) { Rbkit::MESSAGE_FIELDS[:file] }
  let(:line) { Rbkit::MESSAGE_FIELDS[:line] }
  let(:size) { Rbkit::MESSAGE_FIELDS[:size] }
  let(:age) { Rbkit::MESSAGE_FIELDS[:age] }
  let(:references) { Rbkit::MESSAGE_FIELDS[:references] }

  context "capture new objects age" do
    before do
      Rbkit.start_profiling(enable_gc_stats: false, enable_object_trace: true)
    end

    it "should record object generation of new objects" do
      $foo_obj = Foo.new
      snapshot_info = ask_object_dump
      foo_obj_info = extract_obj_info(snapshot_info, "Foo")
      gen_info = foo_obj_info[age]
      puts "Gen info is #{gen_info} and gc_count is #{GC.count}"
      expect(gen_info).to be < 3
      GC.start
      snapshot_info = ask_object_dump
      foo_obj_info = extract_obj_info(snapshot_info, "Foo")
      gen_info2 = foo_obj_info[age]
      puts "After Gen info is #{gen_info2} and gc_count is #{GC.count}"
      expect(gen_info2).to be > gen_info
    end

    after do
      Rbkit.stop_server
    end
  end

  def extract_obj_info(snapshot_info, klass_name)
    snapshot_info[payload].detect { |x| x[class_name] == klass_name }
  end

  def ask_object_dump
    Rbkit.send_objectspace_dump
    packed_message = Rbkit.get_queued_messages
    message_list = MessagePack.unpack(packed_message)
    extract_heap_snapshot(message_list)
  end

  def extract_heap_snapshot(message_list)
    message_list[payload].detect { |hash|
      hash[event_type] == object_dump
    }
  end
end
