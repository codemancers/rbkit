require 'spec_helper'

describe 'Rbkit.start_server' do
  before(:all) do
    Rbkit.start_server
    @foo_obj = Foo.new
    @messages = Rbkit.get_queued_messages
    Rbkit.stop_server
  end
  it 'should not send any messages' do
    expect(@messages).to be_nil
  end
  context 'when pub port is out of range' do
    it 'should raise ArgumentError' do
      expect{ Rbkit.start_server(pub_port: 1023) }
        .to raise_error(ArgumentError, 'Invalid port value')
      expect{ Rbkit.start_server(pub_port: 65001) }
        .to raise_error(ArgumentError, 'Invalid port value')
    end
  end
  context 'when request port is out of range' do
    it 'should raise ArgumentError' do
      expect{ Rbkit.start_server(request_port: 1023) }
        .to raise_error(ArgumentError, 'Invalid port value')
      expect{ Rbkit.start_server(request_port: 65001) }
        .to raise_error(ArgumentError, 'Invalid port value')
    end
  end
end
