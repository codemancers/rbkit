require 'spec_helper'
Rbkit.disable_test_mode

describe 'Callbacks which get triggered when sending messages' do
  before(:all) do
    @published_messages = []
    @response_messages = []
    server = Rbkit.start_server
    server.publish_callback = -> (message) {
      @published_messages << message
    }
    server.respond_callback = -> (message) {
      @response_messages << message
    }

    server.process_incoming_request('handshake')
    server.process_incoming_request('start_memory_profile')
    500.times { Foo.new }
    server.process_incoming_request('stop_memory_profile')
    server.stop
  end

  it 'triggers publish_callback with messages containing only event collections' do
    expect(@published_messages.size).to be > 1
    @published_messages.each do |packed_message|
      message = MessagePack.unpack packed_message
      expect(message[Rbkit::MESSAGE_FIELDS[:event_type]]).to eql Rbkit::EVENT_TYPES[:event_collection]
    end
  end

  it 'triggers respond_callback with messages containing only handshake and acknowledgements' do
    expect(@response_messages.size).to eql 3

    message = MessagePack.unpack @response_messages[0]
    expect(message[Rbkit::MESSAGE_FIELDS[:event_type]]).to eql Rbkit::EVENT_TYPES[:handshake]
    expect(@response_messages[1]).to eql 'ok' # Ack for 'start_memory_profile' command
    expect(@response_messages[2]).to eql 'ok' # Ack for 'stop_memory_profile' command
  end
end

