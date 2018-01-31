require 'spec_helper'
require 'support/cpu_sample_helpers'
include CpuSampleHelpers

describe 'CPU Sampling' do
  let(:sampling_interval_in_usec) { 1000 }

  def io_intensive_operation
    r, w = IO.pipe
    IO.select([r], nil, nil, 1)
  ensure
    r.close
    w.close
  end

  def cpu_intensive_operation
    100000.times{|x| Math.sqrt(x)}
  end

  class SampleClassForTest
    class Sample2
      def baz(block)
        instance_eval 'def zab(block) block.call end'
        [self, zab(block)]
      end
    end

    def self.bar(block)
      Sample2.new.baz(block)
    end

    def foo(block)
      self.class.bar(block)
    end
  end

  before do
    server = Rbkit.start_profiling(enable_gc_stats: false, enable_object_trace: false,
                                   enable_cpu_profiling: true, clock_type: clock_type,
                                   cpu_profiling_mode: :sampling, cpu_sampling_interval_usec: sampling_interval_in_usec)
    Fiber.new {
      Fiber.yield SampleClassForTest.new.foo(operation)
    }.resume
    @messages = MessagePack.unpack server.get_queued_messages
    server.stop
  end

  context 'when using wall time' do
    let(:clock_type) { :wall }
    let(:operation) { lambda{ io_intensive_operation; cpu_intensive_operation; } }

    it 'should record the correct stack frames for io intensive operation' do
      # Make sure sampling can detect most expensive method
      expect(@messages).to have_message(Rbkit::EVENT_TYPES[:cpu_sample])
      expect(@messages).to have_most_cpu_samples_for("#{__FILE__}:8")

      # Make sure stack trace is correct
      frames = most_frequent_sample(@messages)
      expect(frames.size).to eql 7

      frame = frames[0]
      expect(frame[Rbkit::MESSAGE_FIELDS[:method_name]]).to eql 'io_intensive_operation'
      expect(frame[Rbkit::MESSAGE_FIELDS[:label]]).to eql 'RSpec::ExampleGroups::CPUSampling#io_intensive_operation'
      expect(frame[Rbkit::MESSAGE_FIELDS[:file]]).to eql __FILE__
      expect(frame[Rbkit::MESSAGE_FIELDS[:line]]).to eql 8
      expect(frame[Rbkit::MESSAGE_FIELDS[:singleton_method]]).to eql 0
      expect(frame[Rbkit::MESSAGE_FIELDS[:thread_id]]).to eql Thread.current.object_id

      frame = frames[1]
      expect(frame[Rbkit::MESSAGE_FIELDS[:method_name]]).to be_nil
      expect(frame[Rbkit::MESSAGE_FIELDS[:label]]).to eql 'block (4 levels) in <top (required)>'
      expect(frame[Rbkit::MESSAGE_FIELDS[:file]]).to eql __FILE__
      expect(frame[Rbkit::MESSAGE_FIELDS[:line]]).to eql 50
      expect(frame[Rbkit::MESSAGE_FIELDS[:singleton_method]]).to eql 0
      expect(frame[Rbkit::MESSAGE_FIELDS[:thread_id]]).to eql Thread.current.object_id

      frame = frames[2]
      expect(frame[Rbkit::MESSAGE_FIELDS[:method_name]]).to eql 'zab'
      expect(frame[Rbkit::MESSAGE_FIELDS[:label]]).to match /#<SampleClassForTest::Sample2:0x.*>.zab/
      expect(frame[Rbkit::MESSAGE_FIELDS[:file]]).to eql '(eval)'
      expect(frame[Rbkit::MESSAGE_FIELDS[:line]]).to eql 1
      expect(frame[Rbkit::MESSAGE_FIELDS[:singleton_method]]).to eql 1
      expect(frame[Rbkit::MESSAGE_FIELDS[:thread_id]]).to eql Thread.current.object_id

      frame = frames[3]
      expect(frame[Rbkit::MESSAGE_FIELDS[:method_name]]).to eql 'baz'
      expect(frame[Rbkit::MESSAGE_FIELDS[:label]]).to eql 'SampleClassForTest::Sample2#baz'
      expect(frame[Rbkit::MESSAGE_FIELDS[:file]]).to eql __FILE__
      expect(frame[Rbkit::MESSAGE_FIELDS[:line]]).to eql 22
      expect(frame[Rbkit::MESSAGE_FIELDS[:singleton_method]]).to eql 0
      expect(frame[Rbkit::MESSAGE_FIELDS[:thread_id]]).to eql Thread.current.object_id

      frame = frames[4]
      expect(frame[Rbkit::MESSAGE_FIELDS[:method_name]]).to eql 'bar'
      expect(frame[Rbkit::MESSAGE_FIELDS[:label]]).to eql 'SampleClassForTest.bar'
      expect(frame[Rbkit::MESSAGE_FIELDS[:file]]).to eql __FILE__
      expect(frame[Rbkit::MESSAGE_FIELDS[:line]]).to eql 28
      expect(frame[Rbkit::MESSAGE_FIELDS[:singleton_method]]).to eql 1
      expect(frame[Rbkit::MESSAGE_FIELDS[:thread_id]]).to eql Thread.current.object_id

      frame = frames[5]
      expect(frame[Rbkit::MESSAGE_FIELDS[:method_name]]).to eql 'foo'
      expect(frame[Rbkit::MESSAGE_FIELDS[:label]]).to eql 'SampleClassForTest#foo'
      expect(frame[Rbkit::MESSAGE_FIELDS[:file]]).to eql __FILE__
      expect(frame[Rbkit::MESSAGE_FIELDS[:line]]).to eql 32
      expect(frame[Rbkit::MESSAGE_FIELDS[:singleton_method]]).to eql 0
      expect(frame[Rbkit::MESSAGE_FIELDS[:thread_id]]).to eql Thread.current.object_id

      frame = frames[6]
      expect(frame[Rbkit::MESSAGE_FIELDS[:method_name]]).to be_nil
      expect(frame[Rbkit::MESSAGE_FIELDS[:label]]).to eql 'block (3 levels) in <top (required)>'
      expect(frame[Rbkit::MESSAGE_FIELDS[:file]]).to eql __FILE__
      expect(frame[Rbkit::MESSAGE_FIELDS[:line]]).to eql 41
      expect(frame[Rbkit::MESSAGE_FIELDS[:singleton_method]]).to eql 0
      expect(frame[Rbkit::MESSAGE_FIELDS[:thread_id]]).to eql Thread.current.object_id
    end
  end

  context 'when using cpu time' do
    let(:clock_type) { :cpu }
    let(:operation) { lambda{ io_intensive_operation; cpu_intensive_operation; } }

    # TODO: cpu sampling is listing name of enclosing method in 2.3.x
    xit 'should record the correct stack frames for cpu instensive operation' do
      expect(@messages).to have_message(Rbkit::EVENT_TYPES[:cpu_sample])
      expect(@messages).to have_most_cpu_samples_for("#{__FILE__}:17")

      # Make sure stack trace is correct
      frames = most_frequent_sample(@messages)
      expect(frames.size).to eql 8

      frame = frames[0]
      expect(frame[Rbkit::MESSAGE_FIELDS[:method_name]]).to eql 'cpu_intensive_operation'
      expect(frame[Rbkit::MESSAGE_FIELDS[:label]]).to eql 'block in RSpec::ExampleGroups::CPUSampling#cpu_intensive_operation'
      expect(frame[Rbkit::MESSAGE_FIELDS[:file]]).to eql __FILE__
      expect(frame[Rbkit::MESSAGE_FIELDS[:line]]).to eql 17
      expect(frame[Rbkit::MESSAGE_FIELDS[:singleton_method]]).to eql 0
      expect(frame[Rbkit::MESSAGE_FIELDS[:thread_id]]).to eql Thread.current.object_id

      frame = frames[1]
      expect(frame[Rbkit::MESSAGE_FIELDS[:method_name]]).to eql 'cpu_intensive_operation'
      expect(frame[Rbkit::MESSAGE_FIELDS[:label]]).to eql 'RSpec::ExampleGroups::CPUSampling#cpu_intensive_operation'
      expect(frame[Rbkit::MESSAGE_FIELDS[:file]]).to eql __FILE__
      expect(frame[Rbkit::MESSAGE_FIELDS[:line]]).to eql 16
      expect(frame[Rbkit::MESSAGE_FIELDS[:singleton_method]]).to eql 0
      expect(frame[Rbkit::MESSAGE_FIELDS[:thread_id]]).to eql Thread.current.object_id

      frame = frames[2]
      expect(frame[Rbkit::MESSAGE_FIELDS[:method_name]]).to be_nil
      expect(frame[Rbkit::MESSAGE_FIELDS[:label]]).to eql 'block (4 levels) in <top (required)>'
      expect(frame[Rbkit::MESSAGE_FIELDS[:file]]).to eql __FILE__
      expect(frame[Rbkit::MESSAGE_FIELDS[:line]]).to eql 121
      expect(frame[Rbkit::MESSAGE_FIELDS[:singleton_method]]).to eql 0
      expect(frame[Rbkit::MESSAGE_FIELDS[:thread_id]]).to eql Thread.current.object_id

      frame = frames[3]
      expect(frame[Rbkit::MESSAGE_FIELDS[:method_name]]).to eql 'zab'
      expect(frame[Rbkit::MESSAGE_FIELDS[:label]]).to match /#<SampleClassForTest::Sample2:0x.*>.zab/
      expect(frame[Rbkit::MESSAGE_FIELDS[:file]]).to eql '(eval)'
      expect(frame[Rbkit::MESSAGE_FIELDS[:line]]).to eql 1
      expect(frame[Rbkit::MESSAGE_FIELDS[:singleton_method]]).to eql 1
      expect(frame[Rbkit::MESSAGE_FIELDS[:thread_id]]).to eql Thread.current.object_id

      frame = frames[4]
      expect(frame[Rbkit::MESSAGE_FIELDS[:method_name]]).to eql 'baz'
      expect(frame[Rbkit::MESSAGE_FIELDS[:label]]).to eql 'SampleClassForTest::Sample2#baz'
      expect(frame[Rbkit::MESSAGE_FIELDS[:file]]).to eql __FILE__
      expect(frame[Rbkit::MESSAGE_FIELDS[:line]]).to eql 22
      expect(frame[Rbkit::MESSAGE_FIELDS[:singleton_method]]).to eql 0
      expect(frame[Rbkit::MESSAGE_FIELDS[:thread_id]]).to eql Thread.current.object_id

      frame = frames[5]
      expect(frame[Rbkit::MESSAGE_FIELDS[:method_name]]).to eql 'bar'
      expect(frame[Rbkit::MESSAGE_FIELDS[:label]]).to eql 'SampleClassForTest.bar'
      expect(frame[Rbkit::MESSAGE_FIELDS[:file]]).to eql __FILE__
      expect(frame[Rbkit::MESSAGE_FIELDS[:line]]).to eql 28
      expect(frame[Rbkit::MESSAGE_FIELDS[:singleton_method]]).to eql 1
      expect(frame[Rbkit::MESSAGE_FIELDS[:thread_id]]).to eql Thread.current.object_id

      frame = frames[6]
      expect(frame[Rbkit::MESSAGE_FIELDS[:method_name]]).to eql 'foo'
      expect(frame[Rbkit::MESSAGE_FIELDS[:label]]).to eql 'SampleClassForTest#foo'
      expect(frame[Rbkit::MESSAGE_FIELDS[:file]]).to eql __FILE__
      expect(frame[Rbkit::MESSAGE_FIELDS[:line]]).to eql 32
      expect(frame[Rbkit::MESSAGE_FIELDS[:singleton_method]]).to eql 0
      expect(frame[Rbkit::MESSAGE_FIELDS[:thread_id]]).to eql Thread.current.object_id

      frame = frames[7]
      expect(frame[Rbkit::MESSAGE_FIELDS[:method_name]]).to be_nil
      expect(frame[Rbkit::MESSAGE_FIELDS[:label]]).to eql 'block (3 levels) in <top (required)>'
      expect(frame[Rbkit::MESSAGE_FIELDS[:file]]).to eql __FILE__
      expect(frame[Rbkit::MESSAGE_FIELDS[:line]]).to eql 41
      expect(frame[Rbkit::MESSAGE_FIELDS[:singleton_method]]).to eql 0
      expect(frame[Rbkit::MESSAGE_FIELDS[:thread_id]]).to eql Thread.current.object_id
    end
  end
end
