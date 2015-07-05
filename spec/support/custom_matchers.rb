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

RSpec::Matchers.define :have_most_cpu_samples_for do |label|
  match do |unpacked_message_list|
    messages = unpacked_message_list[Rbkit::MESSAGE_FIELDS[:payload]].select do |event|
      event[Rbkit::MESSAGE_FIELDS[:event_type]] == Rbkit::EVENT_TYPES[:cpu_sample]
    end
    @label_with_most_samples, @sample_count = messages.collect_concat{|x| x[Rbkit::MESSAGE_FIELDS[:payload]].first} # Create a list of all methods where time was spent
      .inject(Hash.new(0)) do |h, frame_data|
        path = frame_data[Rbkit::MESSAGE_FIELDS[:file]] + ':' + frame_data[Rbkit::MESSAGE_FIELDS[:line]].to_s
        h[path] += 1
        h
      end# Create a map with count of each path where method is defined
      .max_by{|k,v| v} # Find path with highest count
    @label_with_most_samples == label
  end

  failure_message do |unpacked_message_list|
    "Expected the most number of cpu samples to be for '#{label}', but are for '#{@label_with_most_samples}' (#{@sample_count} samples)"
  end
end
