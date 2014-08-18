require 'spec_helper'
describe "Object space dump" do
  it "should #TODO" do
    Rbkit.start_profiling
    Object.new
    Object.new
    puts Rbkit.get_queued_messages
  end
end
