module CpuSampleHelpers
  def most_frequent_sample(messages)
    sample_messages = messages[Rbkit::MESSAGE_FIELDS[:payload]].select do |event|
      event[Rbkit::MESSAGE_FIELDS[:event_type]] == Rbkit::EVENT_TYPES[:cpu_sample]
    end

    method_with_most_samples = method_with_most_samples(sample_messages)
    sample_messages.find do |message|
      method_info = message[Rbkit::MESSAGE_FIELDS[:payload]].first
      method_path = method_info[Rbkit::MESSAGE_FIELDS[:file]] + ':' + method_info[Rbkit::MESSAGE_FIELDS[:line]].to_s
      if method_path == method_with_most_samples
        return message[Rbkit::MESSAGE_FIELDS[:payload]]
      end
    end
  end

  def method_with_most_samples(sample_messages)
    sample_messages.collect_concat{|x| x[Rbkit::MESSAGE_FIELDS[:payload]].first} # Create a list of all methods where time was spent
      .inject(Hash.new(0)) do |h, frame_data|
        path = frame_data[Rbkit::MESSAGE_FIELDS[:file]] + ':' + frame_data[Rbkit::MESSAGE_FIELDS[:line]].to_s
        h[path] += 1
        h
      end# Create a map with sampe count, for each method
      .max_by{|k,v| v}[0] # Find path with highest count
  end
end
