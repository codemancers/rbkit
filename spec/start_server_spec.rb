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
end

