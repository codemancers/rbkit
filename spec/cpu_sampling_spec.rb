require 'spec_helper'
require 'msgpack'

describe 'CPU Sampling' do
  let(:sampling_interval_in_usec) { 1000 }

  def idle_method
    r, w = IO.pipe
    IO.select([r], nil, nil, 0.5)
  ensure
    r.close
    w.close
  end

  def find_many_square_roots
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
    let(:operation) { lambda{ idle_method; find_many_square_roots; } }
    it 'should collect most samples for expensive idling method' do
      expect(@messages).to have_message(Rbkit::EVENT_TYPES[:cpu_sample])
      expect(@messages).to have_most_cpu_samples_for("#{__FILE__}:7")
    end
  end

  context 'when using cpu time' do
    let(:clock_type) { :cpu }
    let(:operation) { lambda{ idle_method; find_many_square_roots; } }
    it 'should collect most samples for expensive cpu intensive operation' do
      expect(@messages).to have_message(Rbkit::EVENT_TYPES[:cpu_sample])
      expect(@messages).to have_most_cpu_samples_for("#{__FILE__}:16")
    end
  end
end
