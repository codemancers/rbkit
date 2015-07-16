require 'support/cpu_sample_helpers'
include CpuSampleHelpers

RSpec::Matchers.define :have_message do |event_type|
  match do |unpacked_message_list|
    messages = unpacked_message_list[Rbkit::MESSAGE_FIELDS[:payload]].select do |event|
      event[Rbkit::MESSAGE_FIELDS[:event_type]] == event_type
    end

    if @count
      @size = messages.size
      messages.size == @count
    else
      !messages.empty?
    end
  end

  failure_message do |unpacked_message_list|
    if @count
      "Expected message to contain #{@count} #{event_type} event(s) but contains #{@size}."
    else
      "Expected message to contain atleast one #{event_type} event."
    end
  end

  chain :with_count do |count|
    @count = count
  end
end

RSpec::Matchers.define :have_most_cpu_samples_for do |method_path|
  match do |unpacked_message_list|
    messages = unpacked_message_list[Rbkit::MESSAGE_FIELDS[:payload]].select do |event|
      event[Rbkit::MESSAGE_FIELDS[:event_type]] == Rbkit::EVENT_TYPES[:cpu_sample]
    end
    @method_with_most_samples = method_with_most_samples(messages)
    method_path == @method_with_most_samples
  end

  failure_message do |unpacked_message_list|
    "Expected the most number of cpu samples to be for '#{method_path}', but are for '#{@method_with_most_samples}'"
  end
end
