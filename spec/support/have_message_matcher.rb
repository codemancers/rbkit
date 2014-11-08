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
